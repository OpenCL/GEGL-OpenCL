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
 * You should clean received a copy of the GNU Lesser General Public
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

#include "gegl-clean-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-utils.h"
#include "gegl-visitable.h"
#include "gegl-instrument.h"

static void gegl_clean_visitor_class_init (GeglCleanVisitorClass *klass);
static void visit_node                   (GeglVisitor          *self,
                                          GeglNode             *node);


G_DEFINE_TYPE(GeglCleanVisitor, gegl_clean_visitor, GEGL_TYPE_VISITOR)


static void
gegl_clean_visitor_class_init (GeglCleanVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_clean_visitor_init (GeglCleanVisitor *self)
{
}

static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
  GeglRect rect = {0,0,0,0};

  GEGL_VISITOR_CLASS (gegl_clean_visitor_parent_class)->visit_node (self, node);
  if (!node)
    return;

  node->dirt_rect = rect;
}
