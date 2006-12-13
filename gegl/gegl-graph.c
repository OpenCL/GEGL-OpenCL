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
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>
#include "gegl-types.h"
#include "gegl-graph.h"
#include "gegl-node.h"
#include "gegl-visitable.h"
#include "gegl-pad.h"
#include "gegl-visitor.h"
#include "gegl-connection.h"
#include "gegl-node.h"

static void gegl_graph_class_init (GeglGraphClass *klass);
static void gegl_graph_init       (GeglGraph      *self);
static void dispose               (GObject        *object);

G_DEFINE_TYPE(GeglGraph, gegl_graph, GEGL_TYPE_OBJECT)


static void
gegl_graph_class_init (GeglGraphClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = dispose;
}

static void
gegl_graph_init (GeglGraph *self)
{
  self->children = NULL;
}

static void
dispose (GObject *object)
{
  GeglGraph *self = GEGL_GRAPH (object);
  
  gegl_graph_remove_children (self);

  G_OBJECT_CLASS (gegl_graph_parent_class)->dispose (object);
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

/*
 * Returns a copy of the graphs internal list of nodes
 */
GList *
gegl_graph_get_children (GeglGraph *self)
{
  g_return_val_if_fail (GEGL_IS_GRAPH (self), NULL);

  return g_list_copy (self->children);
}

/*
 *  returns a freshly created node, owned by the graph, and thus freed with it
 */
GeglNode *
gegl_graph_new_node (GeglNode    *node,
                     const gchar *first_property_name,
                     ...)
{
  GeglGraph   *self = GEGL_GRAPH (node);
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

GeglNode * gegl_graph_create_node      (GeglNode     *self,
                                        const gchar  *operation)
{
  return gegl_graph_new_node (self, "operation", operation, NULL);
}

static void
source_invalidated (GeglNode      *source,
                    GeglRectangle *rect,
                    gpointer       data)
{
  GeglRectangle dirty_rect;
  GeglNode *destination = GEGL_NODE (data);
  gchar *source_name;
  gchar *destination_name;

  destination_name = g_strdup (gegl_node_get_debug_name (destination));
  source_name = g_strdup (gegl_node_get_debug_name (source));

  if(0)g_warning ("graph:%s is dirtied from %s (%i,%i %ix%i)",
     destination_name,
     source_name,
     rect->x, rect->y,
     rect->w, rect->h);

  dirty_rect = *rect;

  g_signal_emit (destination, gegl_node_signals[GEGL_NODE_INVALIDATED], 0, &dirty_rect, NULL);

  g_free (source_name);
  g_free (destination_name);
}


static GeglNode *
gegl_graph_get_pad_proxy (GeglGraph   *graph,
                          const gchar *name,
                          gboolean     is_graph_input)
{
  GeglNode *node = GEGL_NODE (graph);
  GeglPad  *pad;

  pad = gegl_node_get_pad (node, name);
  if (!pad)
    {
        GeglNode *nop = g_object_new (GEGL_TYPE_NODE, "operation", "nop", "name", is_graph_input?"proxynop-input":"proxynop-output", NULL);
        GeglPad  *nop_pad = gegl_node_get_pad (nop, is_graph_input?"input":"output");
        gegl_graph_add_child (graph, nop);
        
        {
          GeglPad *new_pad = g_object_new (GEGL_TYPE_PAD, NULL);
          gegl_pad_set_param_spec (new_pad, nop_pad->param_spec);
          gegl_pad_set_node (new_pad, nop);
          gegl_object_set_name (GEGL_OBJECT (new_pad), name);
          gegl_node_add_pad (node, new_pad);
          if (!strcmp (name, "aux"))
            {
              g_object_set_data (G_OBJECT (nop), "is-aux", "foo");
            }
        }

        g_object_set_data (G_OBJECT (nop), "graph", graph);
        node->is_graph = TRUE;

        if (!is_graph_input)
          {
            g_signal_connect (G_OBJECT (nop), "invalidated",
                G_CALLBACK (source_invalidated), graph);
          }
        return nop;
    }
  return gegl_pad_get_node (pad);
}

GeglNode *
gegl_graph_input (GeglNode    *node,
                  const gchar *name)
{
  return gegl_graph_get_pad_proxy (GEGL_GRAPH (node), name, TRUE);
}

GeglNode *
gegl_graph_output (GeglNode    *node,
                  const gchar *name)
{
  return gegl_graph_get_pad_proxy (GEGL_GRAPH (node), name, FALSE);
}

GeglNode *
gegl_graph_new (void)
{
  return g_object_new (GEGL_TYPE_GRAPH, NULL);
}
