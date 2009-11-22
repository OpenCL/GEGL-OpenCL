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
#include "gegl-prepare-visitor.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "gegl-instrument.h"
#include "operation/gegl-operation.h"


static void gegl_prepare_visitor_class_init (GeglPrepareVisitorClass *klass);
static void gegl_prepare_visitor_visit_node (GeglVisitor             *self,
                                             GeglNode                *node);


G_DEFINE_TYPE (GeglPrepareVisitor, gegl_prepare_visitor, GEGL_TYPE_VISITOR)


static void
gegl_prepare_visitor_class_init (GeglPrepareVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = gegl_prepare_visitor_visit_node;
}

static void
gegl_prepare_visitor_init (GeglPrepareVisitor *self)
{
}

/* adds a context to the node, calls the operation's prepare method and
 * sets the node's "needed rectangle" to an empty one
 */
static void
gegl_prepare_visitor_visit_node (GeglVisitor *self,
                                 GeglNode    *node)
{
  GeglOperation *operation = node->operation;

  glong          time = gegl_ticks ();

  /* call the parent's class (gegl-visitor.c) visit_node function */
  GEGL_VISITOR_CLASS (gegl_prepare_visitor_parent_class)->visit_node (self, node);

  if (self->context_id == NULL)
    g_warning ("hmm");
  gegl_node_add_context (node, self->context_id);

  /* prepare the operation for the coming evaluation (all properties
   * should be set now).
   */
  {
    const gchar *name = gegl_node_get_name (node);
    if (name && !strcmp (name, "proxynop-output"))
      {
        GeglGraph *graph = g_object_get_data (G_OBJECT (node), "graph");
        g_assert (graph);
        if (GEGL_NODE (graph)->operation)
          {
#if ENABLE_MT
            g_mutex_lock (GEGL_NODE (graph)->mutex);
#endif
            /* issuing a prepare on the graph, FIXME: we might need to do
             * a cycle of prepares as deep as the nesting of graphs,.
             * (or find a better way to do this) */
            gegl_operation_prepare (GEGL_NODE (graph)->operation);
#if ENABLE_MT
            g_mutex_unlock (GEGL_NODE (graph)->mutex);
#endif
          }
      }
  }

#if ENABLE_MT
  g_mutex_lock (node->mutex);
#endif
  gegl_operation_prepare (operation);
#if ENABLE_MT
  g_mutex_unlock (node->mutex);
#endif
  {
    /* initialise the "needed rectangle" to an empty one */
    GeglRectangle empty ={0,};
    gegl_node_set_need_rect (node, self->context_id, &empty);
  }
  time = gegl_ticks () - time;
  gegl_instrument ("process", gegl_node_get_operation (node), time);
  gegl_instrument (gegl_node_get_operation (node), "prepare", time);
}
