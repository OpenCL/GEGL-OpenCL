
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

#include "gegl-graph.h"
#include "gegl-visitable.h"
#include "gegl-property.h"
#include "gegl-visitor.h"

enum
{
  PROP_0,
  PROP_LAST
};

static void gegl_graph_class_init (GeglGraphClass *klass);
static void gegl_graph_init       (GeglGraph      *self);
static void finalize              (GObject        *gobject);
static void set_property          (GObject        *gobject,
                                   guint           prop_id,
                                   const GValue   *value,
                                   GParamSpec     *pspec);
static void get_property          (GObject        *gobject,
                                   guint           prop_id,
                                   GValue         *value,
                                   GParamSpec     *pspec);


G_DEFINE_TYPE(GeglGraph, gegl_graph, GEGL_TYPE_NODE)


static void
gegl_graph_class_init (GeglGraphClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
}

static void
gegl_graph_init (GeglGraph *self)
{
  self->children = NULL;
}

static void
finalize(GObject *gobject)
{
  GeglGraph *self = GEGL_GRAPH (gobject);
  GeglNode  *node = GEGL_NODE (self);

  while (node->properties)
    {
      GeglProperty *property = g_list_nth_data (node->properties, 0);

      node->properties = g_list_remove (node->properties, property);
    }

  gegl_graph_remove_children (self);

  G_OBJECT_CLASS (gegl_graph_parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      break;
  }
}

void
gegl_graph_remove_children(GeglGraph *self)
{
  g_return_if_fail (GEGL_IS_GRAPH (self));
  while(1)
    {
      GeglNode *child = gegl_graph_get_nth_child(self, 0);

      if(child)
        gegl_graph_remove_child(self, child);
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
  self->children = g_list_append(self->children, child);
  g_object_ref(child);
  return child;
}

GeglNode *
gegl_graph_remove_child (GeglGraph *self,
                         GeglNode  *child)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (child), NULL);
  self->children = g_list_remove(self->children, child);
  g_object_unref(child);
  return child;
}

gint
gegl_graph_num_children (GeglGraph *self)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), -1);
  return g_list_length(self->children);
}


GeglNode *
gegl_graph_get_nth_child (GeglGraph *self,
                          gint       n)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  return g_list_nth_data(self->children, n);
}

GList *
gegl_graph_get_children  (GeglGraph *self)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);
  return self->children;
}
