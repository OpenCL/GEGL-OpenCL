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
#include <string.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-finish-visitor.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "gegl-instrument.h"
#include "operation/gegl-operation.h"


static void gegl_finish_visitor_class_init (GeglFinishVisitorClass *klass);
static void gegl_finish_visitor_visit_node (GeglVisitor            *self,
                                            GeglNode               *node);


G_DEFINE_TYPE (GeglFinishVisitor, gegl_finish_visitor, GEGL_TYPE_VISITOR)


static void
gegl_finish_visitor_class_init (GeglFinishVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = gegl_finish_visitor_visit_node;
}

static void
gegl_finish_visitor_init (GeglFinishVisitor *self)
{
}

static void
gegl_finish_visitor_visit_node (GeglVisitor *self,
                                GeglNode    *node)
{
  GEGL_VISITOR_CLASS (gegl_finish_visitor_parent_class)->visit_node (self, node);

  gegl_node_remove_context (node, self->context_id);
}
