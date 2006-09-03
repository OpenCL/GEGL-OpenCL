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
 *           2005, 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-utils.h"

static void      gegl_operation_class_init (GeglOperationClass    *klass);
static void      gegl_operation_init       (GeglOperation         *self);
static void      associate                 (GeglOperation         *self);
static void      clean_pads                (GeglOperation *self);

static GeglRect get_defined_region (GeglOperation *self);
static gboolean calc_source_regions (GeglOperation *self);
static gboolean calc_result_rect (GeglOperation *self);

G_DEFINE_TYPE (GeglOperation, gegl_operation, G_TYPE_OBJECT)

static void
gegl_operation_class_init (GeglOperationClass * klass)
{
  klass->name = NULL;  /* an operation class with name == NULL is not included
                          when doing operation lookup by name */
  klass->description = NULL;

  klass->associate = associate;
  klass->prepare = NULL;
  klass->clean_pads = clean_pads;
  klass->get_defined_region = get_defined_region;
  klass->calc_source_regions = calc_source_regions;
  klass->calc_result_rect = calc_result_rect;
}

static void
gegl_operation_init (GeglOperation *self)
{
}

/**
 * gegl_operation_create_pad:
 * @self: a #GeglOperation.
 * @param_spec:
 *
 * Create a property.
 **/
void
gegl_operation_create_pad (GeglOperation *self,
                           GParamSpec    *param_spec)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (param_spec);

  if (!self->node)
    {
      g_warning ("gegl_operation_create_pad aborting, no associated node, this"
                 " should be called after the operation is associated with a specific node.");
      return;
    }

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self->node);
  gegl_node_add_pad (self->node, pad);
}

gboolean
gegl_operation_process (GeglOperation *self,
                        const gchar   *output_pad)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (self);

  return klass->process (self, output_pad);
}

GeglRect
gegl_operation_get_defined_region (GeglOperation *self)
{
  GeglRect rect = {0,0,0,0};
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);
  g_warning ("'%s'.get_defined_region",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  if (klass->get_defined_region)
    return klass->get_defined_region (self);
  return rect;
}

gboolean
gegl_operation_calc_source_regions (GeglOperation *self)
{
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->calc_source_regions)
    return klass->calc_source_regions (self);
  return FALSE;
}

gboolean
gegl_operation_calc_result_rect (GeglOperation *self)
{
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->calc_result_rect)
    return klass->calc_result_rect (self);
  return FALSE;
}

#include <stdio.h>

static void
associate (GeglOperation *self)
{
  fprintf (stderr, "kilroy was at What The Hack (%p)\n", (void*)self);
  return;
}

void
gegl_operation_associate (GeglOperation *self,
                          GeglNode      *node)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  klass = GEGL_OPERATION_GET_CLASS (self);

  g_assert (klass->associate);
  self->node = node;
  klass->associate (self);
}

void
gegl_operation_prepare (GeglOperation *self)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self);
}



/* this virtual method is here to clean up (unref) output
 * pads when all other nodes that depend on the data on
 * an output pad have used the data
 */
static void
clean_pads (GeglOperation *self)
{
  g_warning ("Don't know how to clean pads of %s", gegl_node_get_debug_name (self->node));
  return;
}

void
gegl_operation_clean_pads (GeglOperation *self)
{
  GeglOperationClass *klass;
  g_return_if_fail (GEGL_IS_OPERATION (self));
  klass = GEGL_OPERATION_GET_CLASS (self);
  g_assert (klass->clean_pads);
  klass->clean_pads (self);
}

void
gegl_operation_set_have_rect (GeglOperation *operation,
                              gint           x,
                              gint           y,
                              gint           width,
                              gint           height)
{
  g_assert (operation);
  g_assert (operation->node);
  gegl_node_set_have_rect (operation->node, x, y, width, height);
}

GeglRect *
gegl_operation_get_have_rect (GeglOperation *operation,
                              const gchar   *input_pad_name)
{
  GeglPad *pad;
  g_assert (operation &&
            operation->node &&
            input_pad_name);
  pad = gegl_node_get_pad (operation->node, input_pad_name);

  if (!pad)
    return NULL;
  pad = gegl_pad_get_connected_to (pad);

  if (!pad)
    return NULL;
  
  g_assert (gegl_pad_get_node (pad));

  return gegl_node_get_have_rect (gegl_pad_get_node (pad));
}

static GList *
producer_nodes (GeglPad *input_pad)
{
  GList *ret = NULL;
  GList *connections;
  g_return_val_if_fail (GEGL_IS_PAD (input_pad), NULL);

  g_assert (gegl_pad_is_input (input_pad));

  connections = gegl_pad_get_connections (input_pad);
  while (connections)
    {
      GeglNode *node = gegl_pad_get_node (gegl_connection_get_source_prop (connections->data));

      if (node)
        ret = g_list_append (ret, node);
      connections = g_list_next (connections);
    }

  if (!strcmp (gegl_object_get_name (GEGL_OBJECT (gegl_pad_get_node (input_pad))), "proxynop-input"))
    {
      GeglNode *self = gegl_pad_get_node (input_pad);
      GList *llink;

      self = GEGL_NODE (g_object_get_data (G_OBJECT (self), "graph"));

      for (llink = self->sources; llink; llink = g_list_next (llink))
        {
          GeglConnection *connection = llink->data;
          GeglNode *source_node = gegl_pad_get_node (gegl_connection_get_source_prop (connection));

          if (! g_list_find (ret, source_node))
            ret = g_list_append (ret, source_node);
        }
    }
  return ret;
}

void
gegl_operation_set_need_rect (GeglOperation *operation,
                              const gchar   *input_pad_name,
                              gint           x,
                              gint           y,
                              gint           width,
                              gint           height)
{
  GeglPad *pad;
  GList   *children;
  GeglRect need = {x, y, width, height},
           child_need;

  g_assert (operation);
  g_assert (operation->node);
  g_assert (input_pad_name);

  pad = gegl_node_get_pad (operation->node, input_pad_name);

  if (!pad && !strcmp (gegl_object_get_name (GEGL_OBJECT (operation->node)), "proxynop-input"))
    {
      GeglGraph *graph = GEGL_GRAPH (g_object_get_data (G_OBJECT (operation->node), "graph"));
      g_assert (graph);
      pad = gegl_node_get_pad (GEGL_NODE (graph), input_pad_name);
    }
  if (!pad)
    {
      g_warning ("buuu %s", gegl_node_get_debug_name (operation->node));
      return;
    }

  children = producer_nodes (pad);

  while (children)
    {
      gegl_rect_bounding_box (&child_need,
                              gegl_node_get_need_rect (children->data), &need);
      gegl_node_set_need_rect (children->data,
                               child_need.x, child_need.y,
                               child_need.w, child_need.h);
      children = g_list_remove (children, children->data);
    }
}

void
gegl_operation_set_result_rect (GeglOperation *operation,
                                gint           x,
                                gint           y,
                                gint           width,
                                gint           height)
{
  g_assert (operation);
  g_assert (operation->node);
  gegl_node_set_result_rect (operation->node, x, y, width, height);
}

static GeglRect
get_defined_region (GeglOperation *self)
{
  GeglRect rect = {0,0,0,0};
  g_warning ("Op '%s' has no proper have_rect function",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  g_warning ("Op '%s' has no proper need_rect function",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return FALSE;
}

#include "gegl/gegl-utils.h"

static gboolean
calc_result_rect (GeglOperation *self)
{
  GeglNode *node = self->node;
  g_assert (node);

  gegl_rect_intersect (&node->result_rect, &node->have_rect, &node->need_rect);
  return TRUE;
}

GeglRect *
gegl_operation_need_rect     (GeglOperation *operation)
{
  g_assert (operation);
  g_assert (operation->node);
  return &operation->node->need_rect;
}
GeglRect *
gegl_operation_have_rect     (GeglOperation *operation)
{
  g_assert (operation);
  g_assert (operation->node);
  return &operation->node->have_rect;
}
GeglRect *
gegl_operation_result_rect   (GeglOperation *operation)
{
  g_assert (operation);
  g_assert (operation->node);
  return &operation->node->result_rect;
}

void
gegl_operation_class_set_name (GeglOperationClass *klass,
                               const gchar        *new_name)
{
  gchar *name_copy;

  name_copy = g_strdup (new_name);
  g_strdelimit (name_copy, "_", '-');
  klass->name = g_intern_string (name_copy);
  g_free (name_copy);
}

void
gegl_operation_class_set_description (GeglOperationClass *klass,
                               const gchar        *new_description)
{
  if (klass->description)
    g_free (klass->description);
  klass->description = g_strdup (new_description);
}

const gchar *
gegl_operation_get_name (GeglOperation *operation)
{
  return GEGL_OPERATION_GET_CLASS (operation)->name;
}
