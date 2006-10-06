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

static GeglRect get_defined_region (GeglOperation *self);
static gboolean calc_source_regions (GeglOperation *self);

G_DEFINE_TYPE (GeglOperation, gegl_operation, G_TYPE_OBJECT)

static void
gegl_operation_class_init (GeglOperationClass * klass)
{
  klass->name = NULL;  /* an operation class with name == NULL is not included
                          when doing operation lookup by name */
  klass->description = NULL;

  klass->associate = associate;
  klass->prepare = NULL;
  klass->get_defined_region = get_defined_region;
  klass->calc_source_regions = calc_source_regions;
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
      g_warning ("gegl_operation_create_pad aborting, no associated node. "
                 "This method should only be called after the operation is "
                 "associated with a node.");
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

static void
associate (GeglOperation *self)
{
  g_warning ("kilroy was at What The Hack (%p, %s)\n", (void*)self,
                         G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
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

GeglRect *
gegl_operation_source_get_defined_region (GeglOperation *operation,
                                          const gchar   *input_pad_name)
{
  GeglPad *pad;
  g_assert (operation &&
            operation->node &&
            input_pad_name);
  pad = gegl_node_get_pad (operation->node, input_pad_name);

  if (!pad)
    return NULL;
  pad = gegl_pad_get_real_connected_to (pad);

  if (!pad)
    return NULL;
  
  g_assert (gegl_pad_get_node (pad));

  return gegl_node_get_have_rect (gegl_pad_get_node (pad));
}

void
gegl_operation_set_source_region (GeglOperation *operation,
                                  const gchar   *input_pad_name,
                                  GeglRect      *region)
{
  GList   *children;
  GeglRect child_need;

  g_assert (operation);
  g_assert (operation->node);
  g_assert (input_pad_name);

  /* FIXME: do not iterate all children like it is currently done, but
   * set the source region on the actual needed node instead, this
   * seems to work for all current ops though.
   */

  children = gegl_node_get_depends_on (operation->node);

  while (children)
    {
      gegl_rect_bounding_box (&child_need,
                              gegl_node_get_need_rect (children->data), region);
      /* expand the need rect of the node, to include what the calling
       * operation needs as well
       */
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
  if (self->node->is_graph)
    {
      return gegl_operation_get_defined_region (
                   gegl_graph_output (self->node, "output")->operation);
    }
  g_warning ("Op '%s' has no defined_region method",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  if (self->node->is_graph)
    {
      return gegl_operation_calc_source_regions (
                         gegl_graph_output (self->node, "output")->operation);
    }

  g_warning ("Op '%s' has no calc_source_regions method",
                         G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return FALSE;
}

GeglRect *
gegl_operation_get_requested_region     (GeglOperation *operation)
{
  g_assert (operation);
  g_assert (operation->node);
  return &operation->node->need_rect;
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
