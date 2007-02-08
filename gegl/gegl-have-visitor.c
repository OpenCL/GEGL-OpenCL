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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"

#include "gegl-have-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"
#include "gegl-instrument.h"

static void gegl_have_visitor_class_init (GeglHaveVisitorClass *klass);
static void visit_node                   (GeglVisitor          *self,
                                          GeglNode             *node);


G_DEFINE_TYPE(GeglHaveVisitor, gegl_have_visitor, GEGL_TYPE_VISITOR)


static void
gegl_have_visitor_class_init (GeglHaveVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_have_visitor_init (GeglHaveVisitor *self)
{
}

static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
  GeglRectangle rect;
  GeglOperation *operation;
  glong    time = gegl_ticks ();

  GEGL_VISITOR_CLASS (gegl_have_visitor_parent_class)->visit_node (self, node);
  if (!node)
    return;
  operation = node->operation;
  if (!operation)
    return;
  rect = gegl_operation_get_defined_region (operation);
  gegl_node_set_have_rect (operation->node, rect.x, rect.y, rect.width, rect.height);

  time = gegl_ticks () - time;
  gegl_instrument ("process", gegl_node_get_operation (node), time);
  gegl_instrument (gegl_node_get_operation (node), "defined-region", time);
}
