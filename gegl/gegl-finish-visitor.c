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
 * You should finish received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"
#include <string.h>
#include <glib-object.h>

#include "gegl-types.h"
#include "gegl-finish-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"
#include "gegl-instrument.h"


static void gegl_finish_visitor_class_init (GeglFinishVisitorClass *klass);
static void visit_node (GeglVisitor *self,
                        GeglNode    *node);


G_DEFINE_TYPE (GeglFinishVisitor, gegl_finish_visitor, GEGL_TYPE_VISITOR)


static void
gegl_finish_visitor_class_init (GeglFinishVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_finish_visitor_init (GeglFinishVisitor *self)
{
}

static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
  GEGL_VISITOR_CLASS (gegl_finish_visitor_parent_class)->visit_node (self, node);

  {
    const gchar *name = gegl_object_get_name (GEGL_OBJECT (node));
    if (name && !strcmp (name, "proxynop-output"))
      {
        GeglGraph *graph = g_object_get_data (G_OBJECT (node), "graph");
        g_assert (graph);
        if (GEGL_NODE (graph)->operation)
          {
            /* issuing a finish on the graph, FIXME: we might need to do
             * a cycle of finishs as deep as the nesting of graphs,.
             * (or find a better way to do this) */

            /* we probably do not need to recurse, prepare should have done that
             * for us..*/
          }
      }
  }

  gegl_node_remove_dynamic (node, self->context_id);
}
