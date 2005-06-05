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

#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-visitor.h"
#include "gegl-visitable.h"
#include "gegl-property.h"
#include "gegl-connection.h"
#include "gegl-eval-mgr.h"


enum
{
  PROP_0
};

static void            gegl_node_class_init     (GeglNodeClass *klass);
static void            gegl_node_init           (GeglNode      *self);
static void            finalize                 (GObject       *self_object);
static void            set_property             (GObject       *gobject,
                                                 guint          prop_id,
                                                 const GValue  *value,
                                                 GParamSpec    *pspec);
static void            get_property             (GObject       *gobject,
                                                 guint          prop_id,
                                                 GValue        *value,
                                                 GParamSpec    *pspec);
static gboolean        properties_exist         (GeglNode      *sink,
                                                 const gchar   *sink_prop_name,
                                                 GeglNode      *source,
                                                 const gchar   *source_prop_name);
static GeglConnection *find_connection          (GeglNode      *sink,
                                                 GeglProperty  *sink_prop);
static void            visitable_init           (gpointer       ginterface,
                                                 gpointer       interface_data);
static void            visitable_accept         (GeglVisitable *visitable,
                                                 GeglVisitor   *visitor);
static GList*          visitable_depends_on     (GeglVisitable *visitable);
static gboolean        visitable_needs_visiting (GeglVisitable *visitable);


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
}

static void
gegl_node_init (GeglNode *self)
{
  self->properties        = NULL;
  self->input_properties  = NULL;
  self->output_properties = NULL;
  self->sinks             = NULL;
  self->sources           = NULL;
  self->enabled           = TRUE;
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

  if (self->properties)
    {
      g_list_foreach (self->properties, (GFunc) g_object_unref, NULL);
      g_list_free (self->properties);
      self->properties = NULL;
    }

  g_list_free (self->input_properties);
  g_list_free (self->output_properties);

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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

/**
 * gegl_node_get_property:
 * @self: a #GeglNode.
 * @name: property name.
 *
 * Get a property.
 *
 * Returns: A #GeglProperty.
 **/
GeglProperty *
gegl_node_get_property (GeglNode    *self,
                        const gchar *name)
{
  GList *list;

  for (list = self->properties; list; list = g_list_next (list))
    {
      GeglProperty *property = list->data;

      if (! strcmp (name, gegl_property_get_name (property)))
        return property;
    }

  return NULL;
}

/**
 * gegl_node_get_properties:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglProperty.
 **/
GList *
gegl_node_get_properties (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->properties;
}

/**
 * gegl_node_get_input_properties:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglProperty.
 **/
GList *
gegl_node_get_input_properties (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->input_properties;
}

/**
 * gegl_node_get_output_properties:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglProperty.
 **/
GList *
gegl_node_get_output_properties (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->output_properties;
}

void
gegl_node_add_property (GeglNode     *self,
                        GeglProperty *property)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PROPERTY (property));

  self->properties = g_list_append (self->properties, property);

  if (gegl_property_is_output (property))
    self->output_properties = g_list_append (self->output_properties, property);

  if (gegl_property_is_input (property))
    self->input_properties = g_list_append (self->input_properties, property);
}

void
gegl_node_remove_property (GeglNode     *self,
                           GeglProperty *property)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PROPERTY (property));

  self->properties = g_list_remove (self->properties, property);

  if (gegl_property_is_output (property))
    self->output_properties = g_list_remove (self->output_properties, property);

  if (gegl_property_is_input (property))
    self->input_properties = g_list_remove (self->input_properties, property);
}

static gboolean
properties_exist (GeglNode    *sink,
                  const gchar *sink_prop_name,
                  GeglNode    *source,
                  const gchar *source_prop_name)
{
  GeglProperty *sink_prop   = gegl_node_get_property (sink, sink_prop_name);
  GeglProperty *source_prop = gegl_node_get_property (source, source_prop_name);

  if (!sink_prop || !gegl_property_is_input (sink_prop))
    {
      g_warning ("Can't find sink property %s", sink_prop_name);
      return FALSE;
    }
  else if (!source_prop || !gegl_property_is_output (source_prop))
    {
      g_warning ("Can't find source property %s", source_prop_name);
      return FALSE;
    }

  return TRUE;
}

static GeglConnection *
find_connection (GeglNode     *sink,
                 GeglProperty *sink_prop)
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

gboolean
gegl_node_connect (GeglNode    *sink,
                   const gchar *sink_prop_name,
                   GeglNode    *source,
                   const gchar *source_prop_name)
{
  g_return_val_if_fail (GEGL_IS_NODE (sink), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (source), FALSE);

  if (properties_exist (sink, sink_prop_name, source, source_prop_name))
    {
      GeglProperty   *sink_prop   = gegl_node_get_property (sink,
                                                            sink_prop_name);
      GeglProperty   *source_prop = gegl_node_get_property (source,
                                                            source_prop_name);
      GeglConnection *connection  = gegl_property_connect (sink_prop,
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

  if (properties_exist (sink, sink_prop_name, source, source_prop_name))
    {
      GeglProperty   *sink_prop   = gegl_node_get_property (sink,
                                                            sink_prop_name);
      GeglProperty   *source_prop = gegl_node_get_property (source,
                                                            source_prop_name);
      GeglConnection *connection  = find_connection (sink, sink_prop);

      if (! connection)
        return FALSE;

      gegl_property_disconnect (sink_prop, source_prop, connection);

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
          GeglProperty *sink_prop = gegl_connection_get_sink_prop (connection);
          GeglProperty *source_prop = gegl_connection_get_source_prop (connection);
          const gchar *sink_prop_name = gegl_property_get_name (sink_prop);
          const gchar *source_prop_name = gegl_property_get_name (source_prop);

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
          GeglProperty *sink_prop = gegl_connection_get_sink_prop (connection);
          GeglProperty *source_prop = gegl_connection_get_source_prop (connection);
          const gchar *sink_prop_name = gegl_property_get_name (sink_prop);
          const gchar *source_prop_name = gegl_property_get_name (source_prop);

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
 * gegl_node_get_num_input_props:
 * @self: a #GeglNode.
 *
 * Gets the number of inputs.
 *
 * Returns: number of inputs.
 **/
gint
gegl_node_get_num_input_props (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->input_properties);
}

/**
 * gegl_node_get_num_output_props:
 * @self: a #GeglNode.
 *
 * Gets the number of outputs.
 *
 * Returns: number of outputs.
 **/
gint
gegl_node_get_num_output_props (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_list_length (self->output_properties);
}

/**
 * gegl_node_get_sinks:
 * @self: a #GeglNode.
 *
 * Gets list of sink connections attached to this node.
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
  GeglNode *node = GEGL_NODE (visitable);

  return gegl_node_get_depends_on (node);
}

static gboolean
visitable_needs_visiting (GeglVisitable *visitable)
{
  return TRUE;
}
