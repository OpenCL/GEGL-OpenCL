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
#include "gegl-types.h"

#include "gegl-node-dynamic.h"

static void            gegl_node_dynamic_class_init           (GeglNodeDynamicClass *klass);
static void            gegl_node_dynamic_init                 (GeglNodeDynamic      *self);

G_DEFINE_TYPE (GeglNodeDynamic, gegl_node_dynamic, G_TYPE_OBJECT);

static void
gegl_node_dynamic_class_init (GeglNodeDynamicClass * klass)
{
  /*GObjectClass *gobject_class = G_OBJECT_CLASS (klass);*/
}

static void
gegl_node_dynamic_init (GeglNodeDynamic *self)
{
  self->refs        = 0;
}

void
gegl_node_dynamic_set_need_rect (GeglNodeDynamic    *node,
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
gegl_node_dynamic_get_result_rect (GeglNodeDynamic *node)
{
  return &node->result_rect;
}

void
gegl_node_dynamic_set_result_rect (GeglNodeDynamic *node,
                                   gint             x,
                                   gint             y,
                                   gint             width,
                                   gint             height)
{
  g_assert (node);
  node->result_rect.x = x;
  node->result_rect.y = y;
  node->result_rect.w = width;
  node->result_rect.h = height;
}

GeglRect *
gegl_node_dynamic_get_need_rect (GeglNodeDynamic    *node)
{
  return &node->need_rect;
}
