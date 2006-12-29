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

#include "gegl-cr-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-node-dynamic.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"
#include "gegl-utils.h"


static void gegl_cr_visitor_class_init (GeglCRVisitorClass *klass);
static void visit_node                 (GeglVisitor        *self,
                                        GeglNode           *node);


G_DEFINE_TYPE(GeglCRVisitor, gegl_cr_visitor, GEGL_TYPE_VISITOR)


static void
gegl_cr_visitor_class_init (GeglCRVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_cr_visitor_init (GeglCRVisitor *self)
{
}

static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
  GeglNodeDynamic *dynamic = gegl_node_get_dynamic (node, self->context_id);
  GEGL_VISITOR_CLASS (gegl_cr_visitor_parent_class)->visit_node (self, node);

  gegl_rect_intersect (&dynamic->result_rect, &node->have_rect, &dynamic->need_rect);

  dynamic->refs = gegl_node_get_num_sinks (node);

  if (!strcmp (gegl_object_get_name (GEGL_OBJECT (node)), "proxynop-output"))
  {
    GeglNode *graph = g_object_get_data (G_OBJECT (node), "graph");
    if (graph)
      dynamic->refs += gegl_node_get_num_sinks (graph);
  }
}
