/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-have-visitor.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "gegl-instrument.h"
#include "operation/gegl-operation.h"

static void gegl_have_visitor_class_init (GeglHaveVisitorClass *klass);
static void gegl_have_visitor_visit_node (GeglVisitor          *self,
                                          GeglNode             *node);


G_DEFINE_TYPE (GeglHaveVisitor, gegl_have_visitor, GEGL_TYPE_VISITOR)


static void
gegl_have_visitor_class_init (GeglHaveVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = gegl_have_visitor_visit_node;
}

static void
gegl_have_visitor_init (GeglHaveVisitor *self)
{
}

/* sets up the node's bounding box */
static void
gegl_have_visitor_visit_node (GeglVisitor *self,
                              GeglNode    *node)
{
  GeglRectangle  rect;
  GeglOperation *operation;
  glong          time = gegl_ticks ();

  GEGL_VISITOR_CLASS (gegl_have_visitor_parent_class)->visit_node (self, node);
  if (!node)
    return;
  operation = node->operation;
#if ENABLE_MT
  g_mutex_lock (node->mutex);
#endif
  rect = gegl_operation_get_bounding_box (operation);

  node->have_rect = rect;
#if ENABLE_MT
  g_mutex_unlock (node->mutex);
#endif

  time = gegl_ticks () - time;
  gegl_instrument ("process", gegl_node_get_operation (node), time);
  gegl_instrument (gegl_node_get_operation (node), "defined-region", time);
}
