/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-operation.h"
#include "gegl-visitor.h"
#include "gegl-visitable.h"
#include "gegl-pad.h"
#include "gegl-connection.h"
#include "gegl-eval-mgr.h"


enum
{
  PROP_0,
  PROP_OPERATION,
  PROP_OPERATION_OBJECT
};

static void            gegl_node_class_init           (GeglNodeClass *klass);
static void            gegl_node_init                 (GeglNode      *self);
static void            finalize                       (GObject       *self_object);
static void            set_property                   (GObject       *gobject,
                                                       guint          prop_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void            get_property                   (GObject       *gobject,
                                                       guint          prop_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);
static gboolean        pads_exist                     (GeglNode      *sink,
                                                       const gchar   *sink_pad_name,
                                                       GeglNode      *source,
                                                       const gchar   *source_pad_name);
static GeglConnection *find_connection                (GeglNode      *sink,
                                                       GeglPad       *sink_pad);
static void            visitable_init                 (gpointer       ginterface,
                                                       gpointer       interface_data);
static void            visitable_accept               (GeglVisitable *visitable,
                                                       GeglVisitor   *visitor);
static GList*          visitable_depends_on           (GeglVisitable *visitable);
static gboolean        visitable_needs_visiting       (GeglVisitable *visitable);
static void            gegl_node_set_operation        (GeglNode      *self,
                                                       const gchar   *operation);
static void            gegl_node_set_operation_object (GeglNode      *self,
                                                       GeglOperation *operation);
static const gchar *   gegl_node_get_operation        (GeglNode      *self);


G_DEFINE_TYPE_WITH_CODE (GeglNode, gegl_node, GEGL_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEGL_TYPE_VISITABLE,
                                                visitable_init))

static void
gegl_node_class_init (GeglNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OPERATION_OBJECT,
                                   g_param_spec_object ("operation-object",
                                   "Operation Object",
                                   "The type of the associated GeglOperation",
                                   GEGL_TYPE_OPERATION,
                                   G_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_OPERATION,
                                   g_param_spec_string ("operation",
                                   "Operation Type",
                                   "The kind of associated GeglOperation",
                                   "",
                                   G_PARAM_CONSTRUCT |
                                   G_PARAM_READWRITE));

}

static void
gegl_node_init (GeglNode *self)
{
  self->pads        = NULL;
  self->input_pads  = NULL;
  self->output_pads = NULL;
  self->sinks       = NULL;
  self->sources     = NULL;
  self->operation   = NULL;
  self->enabled     = TRUE;
}

static void
visitable_init (gpointer ginterface,
                gpointer interface_data)
{
  GeglVisitableClass *visitable_class = ginterface;

  visitable_class->accept         = visitable_accept;
  visitable_class->depends_on     = visitable_depends_on;
  visitable_class->needs_visiting = visitable_needs_visiting;
}

static void
finalize (GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  gegl_node_disconnect_sources (self);
  gegl_node_disconnect_sinks (self);

  if (self->pads)
    {
      g_list_foreach (self->pads, (GFunc) g_object_unref, NULL);
      g_list_free (self->pads);
      self->pads = NULL;
    }

  g_list_free (self->input_pads);
  g_list_free (self->output_pads);

  if (self->operation)
    {
      g_object_unref (self->operation);
      self->operation = NULL;
    }

  G_OBJECT_CLASS (gegl_node_parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_OPERATION:

      gegl_node_set_operation (GEGL_NODE (gobject), g_value_get_string (value));
      break; 
    case PROP_OPERATION_OBJECT:
      gegl_node_set_operation_object (GEGL_NODE (gobject), g_value_get_object (value));
      break; 
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_OPERATION:
      g_value_set_string (value, gegl_node_get_operation (GEGL_NODE (gobject)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

/**
 * gegl_node_get_pad:
 * @self: a #GeglNode.
 * @name: property name.
 *
 * Get a property.
 *
 * Returns: A #GeglPad.
 **/
GeglPad *
gegl_node_get_pad (GeglNode    *self,
                   const gchar *name)
{
  GList *list;

  for (list = self->pads; list; list = g_list_next (list))
    {
      GeglPad *property = list->data;

      if (! strcmp (name, gegl_pad_get_name (property)))
        return property;
    }

  return NULL;
}

/**
 * gegl_node_get_pads:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglPad.
 **/
GList *
gegl_node_get_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->pads;
}

/**
 * gegl_node_get_input_pads:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglPad.
 **/
GList *
gegl_node_get_input_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->input_pads;
}

/**
 * gegl_node_get_output_pads:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglPad.
 **/
GList *
gegl_node_get_output_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->output_pads;
}

void
gegl_node_add_pad (GeglNode *self,
                   GeglPad  *pad)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PAD (pad));

  self->pads = g_list_append (self->pads, pad);

  if (gegl_pad_is_output (pad))
    self->output_pads = g_list_append (self->output_pads, pad);

  if (gegl_pad_is_input (pad))
    self->input_pads = g_list_append (self->input_pads, pad);
}

void
gegl_node_remove_pad (GeglNode *self,
                      GeglPad  *pad)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PAD (pad));

  self->pads = g_list_remove (self->pads, pad);

  if (gegl_pad_is_output (pad))
    self->output_pads = g_list_remove (self->output_pads, pad);

  if (gegl_pad_is_input (pad))
    self->input_pads = g_list_remove (self->input_pads, pad);
}

static gboolean
pads_exist (GeglNode    *sink,
            const gchar *sink_prop_name,
            GeglNode    *source,
            const gchar *source_prop_name)
{
  GeglPad *sink_prop   = gegl_node_get_pad (sink, sink_prop_name);
  GeglPad *source_prop = gegl_node_get_pad (source, source_prop_name);

  if (!sink_prop || !gegl_pad_is_input (sink_prop))
    {
      g_warning ("Can't find sink property %s", sink_prop_name);
      return FALSE;
    }
  else if (!source_prop || !gegl_pad_is_output (source_prop))
    {
      g_warning ("Can't find source property %s", source_prop_name);
      return FALSE;
    }

  return TRUE;
}

static GeglConnection *
find_connection (GeglNode     *sink,
                 GeglPad      *sink_prop)
{
  GList *list;

  g_return_val_if_fail (GEGL_IS_NODE (sink), NULL);

  for (list = sink->sources; list; list = g_list_next (list))
    {
      GeglConnection *connection = list->data;

      if (sink_prop == gegl_connection_get_sink_prop (connection))
        return connection;
    }

  return NULL;
}

#include <stdio.h>

gboolean
gegl_node_connect (GeglNode    *sink,
                   const gchar *sink_prop_name,
                   GeglNode    *source,
                   const gchar *source_prop_name)
{
  g_return_val_if_fail (GEGL_IS_NODE (sink), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (source), FALSE);
  
  if (pads_exist (sink, sink_prop_name, source, source_prop_name))
    {
      GeglPad   *sink_prop   = gegl_node_get_pad (sink,
                                                            sink_prop_name);
      GeglPad   *source_prop = gegl_node_get_pad (source,
                                                            source_prop_name);
      GeglConnection *connection  = gegl_pad_connect (sink_prop,
                                                           source_prop);

      gegl_connection_set_sink_node (connection, sink);
      gegl_connection_set_source_node (connection, source);

      sink->sources = g_list_append (sink->sources, connection);
      source->sinks = g_list_append (source->sinks, connection);

      return TRUE;
    }

  return FALSE;
}

gboolean
gegl_node_disconnect (GeglNode    *sink,
                      const gchar *sink_prop_name,
                      GeglNode    *source,
                      const gchar *source_prop_name)
{
  g_return_val_if_fail (GEGL_IS_NODE (sink), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (source), FALSE);

  if (pads_exist (sink, sink_prop_name, source, source_prop_name))
    {
      GeglPad   *sink_prop   = gegl_node_get_pad (sink,
                                                            sink_prop_name);
      GeglPad   *source_prop = gegl_node_get_pad (source,
                                                            source_prop_name);
      GeglConnection *connection  = find_connection (sink, sink_prop);

      if (! connection)
        return FALSE;

      gegl_pad_disconnect (sink_prop, source_prop, connection);

      sink->sources = g_list_remove (sink->sources, connection);
      source->sinks = g_list_remove (source->sinks, connection);

      g_free (connection);

      return TRUE;
    }

  return FALSE;
}

void
gegl_node_disconnect_sources (GeglNode *self)
{
  while (TRUE)
    {
      GeglConnection *connection = g_list_nth_data (self->sources, 0);

      if (connection)
        {
          GeglNode *sink = gegl_connection_get_sink_node (connection);
          GeglNode *source = gegl_connection_get_source_node (connection);
          GeglPad *sink_prop = gegl_connection_get_sink_prop (connection);
          GeglPad *source_prop = gegl_connection_get_source_prop (connection);
          const gchar *sink_prop_name = gegl_pad_get_name (sink_prop);
          const gchar *source_prop_name = gegl_pad_get_name (source_prop);

          g_assert (self == sink);

          gegl_node_disconnect (sink, sink_prop_name, source, source_prop_name);
        }
      else
        break;
    }
}

void
gegl_node_disconnect_sinks (GeglNode *self)
{
  while (TRUE)
    {
      GeglConnection *connection = g_list_nth_data (self->sinks, 0);

      if (connection)
        {
          GeglNode *sink = gegl_connection_get_sink_node (connection);
          GeglNode *source = gegl_connection_get_source_node (connection);
          GeglPad *sink_prop = gegl_connection_get_sink_prop (connection);
          GeglPad *source_prop = gegl_connection_get_source_prop (connection);
          const gchar *sink_prop_name = gegl_pad_get_name (sink_prop);
          const gchar *source_prop_name = gegl_pad_get_name (source_prop);

          g_assert (self == source);

          gegl_node_disconnect (sink, sink_prop_name, source, source_prop_name);
        }
      else
        break;
    }
}

/**
 * gegl_node_num_sinks:
 * @self: a #GeglNode.
 *
 * Gets the number of sinks
 *
 * Returns: number of sinks
 **/
gint
gegl_node_get_num_sinks (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->sinks);
}

/**
 * gegl_node_get_num_sources:
 * @self: a #GeglNode.
 *
 * Gets the number of source
 *
 * Returns: number of sources
 **/
gint
gegl_node_get_num_sources (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->sources);
}

/**
 * gegl_node_get_num_input_pads:
 * @self: a #GeglNode.
 *
 * Gets the number of inputs.
 *
 * Returns: number of inputs.
 **/
gint
gegl_node_get_num_input_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->input_pads);
}

/**
 * gegl_node_get_num_output_pads:
 * @self: a #GeglNode.
 *
 * Gets the number of outputs.
 *
 * Returns: number of outputs.
 **/
gint
gegl_node_get_num_output_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->output_pads);
}

/**
 * gegl_node_get_sinks:
 * @self: a #GeglNode.
 *
 * Gets list of sink connections attached to this self.
 *
 * Returns: list of sink connections.
 **/
GList *
gegl_node_get_sinks (GeglNode *self)
{
  return self->sinks;
}

/**
 * gegl_node_get_sources:
 * @self: a #GeglNode.
 *
 * Gets list of source connections attached to this node.
 *
 * Returns: list of source connections.
 **/
GList *
gegl_node_get_sources (GeglNode *self)
{
  return self->sources;
}

void
gegl_node_apply (GeglNode    *self,
                 const gchar *output_prop_name)
{
  GeglEvalMgr *eval_mgr;

  g_return_if_fail (GEGL_IS_NODE (self));

  eval_mgr = g_object_new (GEGL_TYPE_EVAL_MGR, NULL);
  gegl_eval_mgr_apply (eval_mgr, self, output_prop_name);
  g_object_unref (eval_mgr);
}

GList *
gegl_node_get_depends_on (GeglNode *self)
{
  GList *depends_on = NULL;
  GList *llink      = self->sources;

  while (llink)
    {
      GeglConnection *connection = llink->data;
      GeglNode * source_node = gegl_connection_get_source_node (connection);

      /* It may already be on the list, so check first */
      if (! g_list_find (depends_on, source_node))
        depends_on = g_list_append (depends_on, source_node);

      llink = g_list_next (llink);
    }

  return depends_on;
}

static void
visitable_accept (GeglVisitable *visitable,
                  GeglVisitor   *visitor)
{
  gegl_visitor_visit_node (visitor, GEGL_NODE (visitable));
}

GList *
visitable_depends_on (GeglVisitable *visitable)
{
  GeglNode *self = GEGL_NODE (visitable);

  return gegl_node_get_depends_on (self);
}

static gboolean
visitable_needs_visiting (GeglVisitable *visitable)
{
  return TRUE;
}

/**
 * gegl_operation_create_property:
 * @self: a #GeglOperation.
 * @param_spec:
 *
 * Create a property.
 **/
void
gegl_node_create_pad (GeglNode   *self,
                      GParamSpec *param_spec)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (param_spec);

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self);
  gegl_node_add_pad (self, pad);
}

gboolean
gegl_node_evaluate (GeglNode    *self,
                    const gchar *output_pad)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), FALSE);
  g_return_val_if_fail (GEGL_IS_OPERATION (self->operation), FALSE);

  return gegl_operation_evaluate (self->operation, output_pad);
}

static void
gegl_node_set_operation (GeglNode      *self,
                         const gchar   *operation_name)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (operation_name);

  if (operation_name && operation_name[0])
    {
      GType         type;
      GeglOperation *operation;

      type = g_type_from_name (operation_name);
      
      if (!type)
        {
          g_warning ("Eeeeek failed to find types %s", operation_name);
          return;
        }

      operation = g_object_new (g_type_from_name (operation_name), NULL);
      gegl_node_set_operation_object (self, operation);
      g_object_unref (operation);
    }
}

static void
gegl_node_set_operation_object (GeglNode      *self,
                                GeglOperation *operation)
{
  g_return_if_fail (GEGL_IS_NODE (self));

  if (!operation)
    return;

  g_return_if_fail (GEGL_IS_OPERATION (operation));

  if (self->operation)
    g_object_unref (self->operation);

  g_object_ref (operation);
  self->operation = operation;
  gegl_operation_associate (operation, self);
}

static const gchar *
gegl_node_get_operation (GeglNode      *self)
{ 
  const gchar *type_name = G_OBJECT_TYPE_NAME (self->operation);
  return &(type_name[0]);
}


void
gegl_node_set (GeglNode    *self,
               const gchar *first_property_name,
               ...)
{
  va_list        var_args;

  g_return_if_fail (GEGL_IS_NODE (self));

  va_start (var_args, first_property_name);
  gegl_node_set_valist (self, first_property_name, var_args);
  va_end (var_args);
}

void
gegl_node_get (GeglNode    *self,
               const gchar *first_property_name,
               ...) 
{
  va_list        var_args;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_OPERATION (self->operation));

  va_start (var_args, first_property_name);
  gegl_node_get_valist (self, first_property_name, var_args);
  va_end (var_args);
}

#include <stdio.h>

void
gegl_node_set_valist (GeglNode     *self,
                      const gchar  *first_property_name,
                      va_list       var_args)
{
  const gchar *property_name;

  g_return_if_fail (GEGL_IS_NODE (self));

  g_object_ref (self);

  g_object_freeze_notify (G_OBJECT (self));

  property_name = first_property_name;
  while (property_name)
    {
      GValue      value = {0, };
      GParamSpec *pspec = NULL;
      gchar      *error = NULL;

      if (!strcmp (property_name, "operation"))
        {
          const gchar *operation_name;
        
          if (first_property_name != property_name)
            {
              g_warning (
                "%s: attempt to change opertation type, and not first property in list.", G_STRFUNC);

              break;
            }
          operation_name = va_arg (var_args, gchar*);
          gegl_node_set_operation (self, operation_name);
        }
      else if (!strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
             G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT(&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (self), property_name, &value);
        }
      else 
        {
          if (self->operation)
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
            }
          if (!pspec)
            {
              g_warning ("%s: operation class '%s' has no property named: '%s'",
               G_STRFUNC,
               G_OBJECT_TYPE_NAME (self->operation),
               property_name);
              break;
            }
          if (!(pspec->flags & G_PARAM_WRITABLE))
            {
              g_warning ("%s: property (%s of operation class '%s' is not writable",
               G_STRFUNC,
               pspec->name,
               G_OBJECT_TYPE_NAME (self->operation));
              break;
            }

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT(&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (self->operation), property_name, &value);
        }

      property_name = va_arg (var_args, gchar*);
    }
  g_object_thaw_notify (G_OBJECT (self));

  g_object_unref (self);
}

void
gegl_node_get_valist (GeglNode    *self,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  const gchar *property_name;

  g_return_if_fail (G_IS_OBJECT (self));

  g_object_ref (self);

  property_name = first_property_name;

  while (property_name)
    {
      GValue value = {0 , };
      GParamSpec *pspec;
      gchar      *error;

      if (!strcmp (property_name, "operation") ||
          !strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
             G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);
        }
      else
        {
        pspec = g_object_class_find_property (
           G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);

        if (!pspec)
          {
            g_warning ("%s: operation class '%s' has no property named '%s'",
             G_STRFUNC,
             G_OBJECT_TYPE_NAME (self->operation),
             property_name);
            break;
          }
        if (!(pspec->flags & G_PARAM_READABLE))
          {
            g_warning ("%s: property '%s' of operation class '%s' is not readable",
             G_STRFUNC,
             property_name,
             G_OBJECT_TYPE_NAME (self->operation));
          }
      }
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

      gegl_node_get_property (self, property_name, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      g_value_unset (&value);

      property_name = va_arg (var_args, gchar*);
    }
  g_object_unref (self);
}

void
gegl_node_set_property (GeglNode     *self,
                        const gchar  *property_name,
                        const GValue *value)
{
  if (!strcmp (property_name, "operation") ||
      !strcmp (property_name, "name"))
    {
      g_object_set_property (G_OBJECT (self),
                             property_name, value);
    }
  else
    {
      if (self->operation)
        {
          g_object_set_property (G_OBJECT (self->operation),
                property_name, value);
        }
    }
}

void
gegl_node_get_property (GeglNode    *self,
                        const gchar *property_name,
                        GValue      *value)
{
  if (!strcmp (property_name, "operation") ||
      !strcmp (property_name, "name")
   )
    {
      g_object_get_property (G_OBJECT (self),
                             property_name, value);
    }
  else
    {
      if (self->operation)
        {
          g_object_get_property (G_OBJECT (self->operation),
                property_name, value);
        }
    }
}

/* returns a freshly allocated list of the properties of the object, does not list
 * the regular gobject properties of GeglNode ('name' and 'operation') */
GParamSpec**
gegl_node_list_properties (GeglNode *self,
                           guint    *n_properties_p)
{
  GParamSpec **pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (self->operation), n_properties_p);

  return pspecs;
}

