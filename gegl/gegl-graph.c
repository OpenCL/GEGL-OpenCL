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

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-graph.h"
#include "gegl-visitable.h"
#include "gegl-pad.h"
#include "gegl-visitor.h"

enum
{
  PROP_0
};

static void gegl_graph_class_init (GeglGraphClass *klass);
static void gegl_graph_init       (GeglGraph      *self);
static void finalize              (GObject        *object);
static void set_property          (GObject        *object,
                                   guint           property_id,
                                   const GValue   *value,
                                   GParamSpec     *pspec);
static void get_property          (GObject        *object,
                                   guint           property_id,
                                   GValue         *value,
                                   GParamSpec     *pspec);


G_DEFINE_TYPE(GeglGraph, gegl_graph, GEGL_TYPE_NODE)


static void
gegl_graph_class_init (GeglGraphClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = finalize;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
}

static void
gegl_graph_init (GeglGraph *self)
{
  self->children = NULL;
}

static void
finalize (GObject *object)
{
  GeglGraph *self = GEGL_GRAPH (object);
  GeglNode  *node = GEGL_NODE (self);

  while (node->pads)
    {
      GeglPad *pad = g_list_nth_data (node->pads, 0);

      node->pads = g_list_remove (node->pads, pad);
    }

  gegl_graph_remove_children (self);

  G_OBJECT_CLASS (gegl_graph_parent_class)->finalize (object);
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
get_property (GObject      *object,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gegl_graph_remove_children (GeglGraph *self)
{
  g_return_if_fail (GEGL_IS_GRAPH (self));

  while (TRUE)
    {
      GeglNode *child = gegl_graph_get_nth_child (self, 0);

      if (child)
        gegl_graph_remove_child (self, child);
      else
        break;
    }
}

GeglNode *
gegl_graph_add_child (GeglGraph *self,
                      GeglNode  *child)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (child), NULL);

  self->children = g_list_append (self->children, g_object_ref (child));

  return child;
}

GeglNode *
gegl_graph_remove_child (GeglGraph *self,
                         GeglNode  *child)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (child), NULL);

  self->children = g_list_remove (self->children, child);
  g_object_unref (child);

  return child;
}

gint
gegl_graph_get_num_children (GeglGraph *self)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), -1);

  return g_list_length (self->children);
}

GeglNode *
gegl_graph_get_nth_child (GeglGraph *self,
                          gint       n)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return g_list_nth_data (self->children, n);
}

GList *
gegl_graph_get_children (GeglGraph *self)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return self->children;
}

/*
 *  returns a freshly created node, owned by the graph, and thus freed with it
 */
GeglNode *
gegl_graph_create_node      (GeglGraph *self,
                             const gchar  *first_property_name,
                             ...)
{
  GeglNode    *node;
  va_list      var_args;
  const gchar *name; 
  
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  node = g_object_new (GEGL_TYPE_NODE, NULL);
  gegl_graph_add_child (self, node);

  name = first_property_name;
  va_start (var_args, first_property_name);
  gegl_node_set_valist (node, first_property_name, var_args);
  va_end (var_args);

  g_object_unref (node);
  return node;
}

