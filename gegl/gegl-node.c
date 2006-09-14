/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
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
#include "buffer/gegl-buffer.h"


enum
{
  PROP_0,
  PROP_OP_CLASS,
  PROP_OPERATION
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
static void            gegl_node_set_operation_object (GeglNode      *self,
                                                       GeglOperation *operation);
static void            gegl_node_set_op_class         (GeglNode      *self,
                                                       const gchar   *op_class,
                                                       const gchar   *first_property,
                                                       va_list        var_args);
static const gchar *   gegl_node_get_op_class         (GeglNode      *self);

G_DEFINE_TYPE_WITH_CODE (GeglNode, gegl_node, GEGL_TYPE_GRAPH,
                         G_IMPLEMENT_INTERFACE (GEGL_TYPE_VISITABLE,
                                                visitable_init))

static void
gegl_node_class_init (GeglNodeClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_OPERATION,
                                   g_param_spec_object ("gegl_operation",
                                   "Operation Object",
                                   "The associated GeglOperation instance",
                                   GEGL_TYPE_OPERATION,
                                   G_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_OP_CLASS,
                                   g_param_spec_string ("operation",
                                   "Operation Type",
                                   "The type of associated GeglOperation",
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
  self->refs        = 0;
  self->enabled     = TRUE;
  self->is_graph    = FALSE;
  self->usecs       = 0;
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
    case PROP_OP_CLASS:
      gegl_node_set_op_class (GEGL_NODE (gobject), g_value_get_string (value),
                              NULL, NULL);
      break;
    case PROP_OPERATION:
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
    case PROP_OP_CLASS:
      g_value_set_string (value, gegl_node_get_op_class (GEGL_NODE (gobject)));
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

  g_assert (self);
  if (!self->pads)
    return NULL;

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

  if (gegl_node_get_pad (self, gegl_pad_get_name (pad)))
    {
      return;
    }
  if(0)g_assert (!gegl_node_get_pad (self, gegl_pad_get_name (pad)));
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

  g_object_unref (pad);
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
      g_warning ("Can't find sink property %s of %s", sink_prop_name,
                  gegl_node_get_debug_name (sink));
      return FALSE;
    }
  else if (!source_prop || !gegl_pad_is_output (source_prop))
    {
      g_warning ("Can't find source property %s of %s", source_prop_name,
                  gegl_node_get_debug_name (source));
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

    {
      GeglPad *pad;
      GeglPad *other_pad = NULL;

      pad = gegl_node_get_pad (sink, sink_prop_name);
      if (pad)
        other_pad = gegl_pad_get_connected_to (pad);
      else
        {
          g_warning ("Didn't find pad '%s' of '%s'", sink_prop_name, gegl_node_get_debug_name (sink));
        }

    if (other_pad)
      {
        gegl_node_disconnect (sink, sink_prop_name, other_pad->node, gegl_pad_get_name (other_pad));
      }
    }
  if (pads_exist (sink, sink_prop_name, source, source_prop_name))
    {
      GeglPad   *sink_prop   = gegl_node_get_pad (sink,   sink_prop_name);
      GeglPad   *source_prop = gegl_node_get_pad (source, source_prop_name);
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
gegl_node_blit_buf (GeglNode    *self,
                    GeglRect    *roi,
                    void        *format,
                    gint         rowstride,
                    gpointer    *destination_buf)
{
  GeglBuffer  *buffer;
  GeglEvalMgr *eval_mgr;

  g_return_if_fail (GEGL_IS_NODE (self));

  g_assert (rowstride == 0);
  eval_mgr = g_object_new (GEGL_TYPE_EVAL_MGR, NULL);
  eval_mgr->roi = *roi;
  gegl_eval_mgr_apply (eval_mgr, self, "output");
  g_object_unref (eval_mgr);
  gegl_node_get (self, "output", &(buffer), NULL);


  {
    GeglBuffer *roi_buf = g_object_new (GEGL_TYPE_BUFFER,
                                        "source", buffer,
                                        "x",      roi->x,
                                        "y",      roi->y,
                                        "width",  roi->w,
                                        "height", roi->h,
                                        NULL);
    gegl_buffer_get_fmt (roi_buf, destination_buf, format);
    g_object_unref (roi_buf);
  }
  /* unref'ing because we used gegl_node_get */
  g_object_unref (buffer);
  /* and unrefing to ultimatly clean it off from the graph */
  g_object_unref (buffer);
}

void
gegl_node_apply_roi (GeglNode    *self,
                     const gchar *output_pad_name,
                     GeglRect    *roi)
{
  GeglEvalMgr *eval_mgr;

  g_return_if_fail (GEGL_IS_NODE (self));
  eval_mgr = g_object_new (GEGL_TYPE_EVAL_MGR, NULL);
  eval_mgr->roi = *roi;
  gegl_eval_mgr_apply (eval_mgr, self, output_pad_name);
  g_object_unref (eval_mgr);
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

#include <stdio.h>
#include "gegl-graph.h"
GList *
gegl_node_get_depends_on (GeglNode *self)
{
  GList *depends_on = NULL;
  GList *llink      = self->sources;

  for (llink = self->sources; llink; llink = g_list_next (llink))
    {
      GeglConnection *connection = llink->data;
      GeglNode * source_node;

      /* this indirection is to make sure we follow the connection correctly
       * if the connected node, is the ghost pad of a graph.
       *
       * Since the node in the graph would be the true recevier of the property.
       */
      source_node = gegl_pad_get_node (gegl_connection_get_source_prop (connection));



      /* It may already be on the list, so check first */
      if (! g_list_find (depends_on, source_node))
        {
          depends_on = g_list_append (depends_on, source_node);
        }
    }

  if (!strcmp (gegl_object_get_name (GEGL_OBJECT (self)), "proxynop-input"))
    {
      GeglGraph *graph = g_object_get_data (G_OBJECT (self), "graph");
      if (graph)
        {
          depends_on = g_list_concat (depends_on, gegl_node_get_depends_on (GEGL_NODE (graph)));
        }
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
GeglPad *
gegl_node_create_pad (GeglNode   *self,
                      GParamSpec *param_spec)
{
  GeglPad *pad;

  if (!GEGL_IS_NODE (self) ||
      !param_spec)
    return NULL;

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self);
  gegl_node_add_pad (self, pad);

  return pad;
}

static GType
g_type_from_op_class2 (GType        parent,
                       const gchar *op_class)
{
  GType        ret = 0;
  GType       *types;
  guint        count;
  gint         no;
  gchar       *op_class_copy;
  const gchar *op_class_intern;

  types = g_type_children (parent, &count);
  if (!types)
    return 0;

  op_class_copy = g_strdup (op_class);
  g_strdelimit (op_class_copy, "_", '-');
  op_class_intern = g_intern_string (op_class_copy);
  g_free (op_class_copy);

  for (no=0; no < count; no++)
    {
      GeglOperationClass *klass = g_type_class_ref (types[no]);

      if (klass->name == op_class_intern)
        {
          ret = types[no];
          g_type_class_unref (klass);
          g_free (types);
          return ret;
        }
      else
        {
          ret = g_type_from_op_class2 (types[no], op_class);
        }
      g_type_class_unref (klass);
      if (ret!=0)
        break;
    }
  g_free (types);
  return ret;
}

static GType
g_type_from_op_class (const gchar *op_class)
{
  return g_type_from_op_class2 (GEGL_TYPE_OPERATION, op_class);
}

static void
gegl_node_set_op_class (GeglNode      *node,
                        const gchar   *op_class,
                        const gchar   *first_property,
                        va_list        var_args)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (op_class);

  if (op_class && op_class[0] != '\0')
    {
      GType         type;
      GeglOperation *operation;

      type = g_type_from_op_class (op_class);

      if (!type)
        {
          g_warning ("Failed to set operation type %s, using a passthrough op instead", op_class);
          if (strcmp (op_class, "nop"))
            gegl_node_set_op_class (node, "nop", NULL, var_args);
          return;
        }

      operation = GEGL_OPERATION (g_object_new_valist (type, first_property,
                                                       var_args));
      gegl_node_set_operation_object (node, operation);
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
gegl_node_get_op_class (GeglNode      *self)
{
  if (!self->operation)
    return NULL;
  return GEGL_OPERATION_GET_CLASS (self->operation)->name;
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
  g_return_if_fail (GEGL_IS_GRAPH (self) || GEGL_IS_OPERATION (self->operation));

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

      /* this code will make sure that all the code that is runing
       * does not break FIXME: remove this after a while, when warnings have
       * died down.
       */
      if (!strcmp (property_name, "class"))
        {
          g_warning ("Setting a deprecated property \"class\" "
                     "use \"operation\" instead");

          property_name = "operation";
        }

      if (!strcmp (property_name, "operation"))
        {
          const gchar *op_class,
                      *op_first_property;

          op_class          = va_arg (var_args, gchar*);
          op_first_property = va_arg (var_args, gchar*);

          /* pass the following properties as construction properties
           * to the operation */
          gegl_node_set_op_class (self, op_class, op_first_property, var_args);
          break;
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
              g_warning ("%s:%s has no property named: '%s'",
               G_STRFUNC, gegl_node_get_debug_name (self), property_name);
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

      if (!strcmp (property_name, "class"))
        {
          g_warning ("Getting a deprecated property \"class\" "
                     "use \"operation\" instead");
          property_name = "operation";
        }

      if (!strcmp (property_name, "operation") ||
          !strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
             G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);
        }
      else
        {
        if (self->is_graph)
          {
            GeglGraph *graph = GEGL_GRAPH (self);
                 pspec = g_object_class_find_property (
                    G_OBJECT_GET_CLASS (G_OBJECT (
                        gegl_graph_output (graph, "output")->operation)), property_name);
            if (!pspec)
              {
                pspec = g_object_class_find_property (
                   G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
              }
          }
        else
          {
            pspec = g_object_class_find_property (
               G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
          }

        if (!pspec)
          {
            g_warning ("%s:%s has no property named: '%s'",
              G_STRFUNC, gegl_node_get_debug_name (self), property_name);
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
  if (!strcmp (property_name, "class"))
    {
      g_warning("Setting a deprecated property \"class\", "
                "use \"operation\" instead.");
      
      g_object_set_property (G_OBJECT (self), "operation", value);
    }

  if (!strcmp (property_name, "operation") ||
      !strcmp (property_name, "name"))
    {
      g_object_set_property (G_OBJECT (self),
                             property_name, value);
    }
  else
    {
      if (self->is_graph)
        {
          g_warning ("set_property for graph,. hmm");
          /* FIXME: should this really be "input")? is_graph doesn't seem to be used,.. */
          g_object_set_property (G_OBJECT (gegl_graph_input (GEGL_GRAPH (self), "input")->operation),
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
    if (self->is_graph &&
        !strcmp (property_name, "output"))
      {
          g_object_get_property (G_OBJECT (gegl_graph_output (GEGL_GRAPH (self), "output")->operation),
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

const gchar *
gegl_node_get_op_type_name    (GeglNode     *node)
{
  if (node == NULL)
    {
      g_warning ("NULL node passed in");
      return "NULL node passd in";
    }
  if (node->is_graph)
    return "GraphNode";
  if (node->operation == NULL)
    {
      g_warning ("No op associated");
      return "No op associated";
    }
  return GEGL_OPERATION_GET_CLASS (node->operation)->name;
}

void
gegl_node_set_have_rect (GeglNode    *node,
                         gint         x,
                         gint         y,
                         gint         width,
                         gint         height)
{
  g_assert (node);
  node->have_rect.x = x;
  node->have_rect.y = y;
  node->have_rect.w = width;
  node->have_rect.h = height;
}

GeglRect *
gegl_node_get_have_rect (GeglNode    *node)
{
  return &node->have_rect;
}

void
gegl_node_set_need_rect (GeglNode    *node,
                         gint         x,
                         gint         y,
                         gint         width,
                         gint         height)
{
  g_assert (node);
  node->need_rect.x = x;
  node->need_rect.y = y;
  node->need_rect.w = width;
  node->need_rect.h = height;
}

GeglRect *
gegl_node_get_result_rect (GeglNode *node)
{
  return &node->result_rect;
}

void
gegl_node_set_result_rect (GeglNode *node,
                           gint      x,
                           gint      y,
                           gint      width,
                           gint      height)
{
  g_assert (node);
  node->result_rect.x = x;
  node->result_rect.y = y;
  node->result_rect.w = width;
  node->result_rect.h = height;
}

GeglRect *
gegl_node_get_need_rect (GeglNode    *node)
{
  return &node->need_rect;
}

const gchar *
gegl_node_get_debug_name (GeglNode     *node)
{
  static gchar ret_buf[512];
  if (gegl_object_get_name (GEGL_OBJECT (node))!=NULL &&
      gegl_object_get_name (GEGL_OBJECT (node))[0] != '\0')
    sprintf (ret_buf, "%s named %s", gegl_node_get_op_type_name (node),
                                     gegl_object_get_name (GEGL_OBJECT (node)));
  else
    sprintf (ret_buf, "%s", gegl_node_get_op_type_name (node));
  return ret_buf;
}
