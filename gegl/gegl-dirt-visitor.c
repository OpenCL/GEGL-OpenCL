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
 * You should dirt received a copy of the GNU Lesser General Public
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

#include "gegl-dirt-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-utils.h"
#include "gegl-visitable.h"
#include "gegl-instrument.h"

static void gegl_dirt_visitor_class_init (GeglDirtVisitorClass *klass);
static void visit_node                   (GeglVisitor          *self,
                                          GeglNode             *node);


G_DEFINE_TYPE(GeglDirtVisitor, gegl_dirt_visitor, GEGL_TYPE_VISITOR)


static void
gegl_dirt_visitor_class_init (GeglDirtVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = visit_node;
}

static void
gegl_dirt_visitor_init (GeglDirtVisitor *self)
{
}

static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
  GeglRect rect = {0,0,0,0};
  GeglOperation *operation;
  glong    time = gegl_ticks ();

  GEGL_VISITOR_CLASS (gegl_dirt_visitor_parent_class)->visit_node (self, node);
  if (!node)
    return;
  operation = node->operation;
  if (!operation)
    return;

  /* should collect dirt from it's sources and unify that dirt with
   * current own dirt
   */
  rect = node->dirt_rect;

  {
    GList *llink;
    for (llink = node->sources; llink; llink = g_list_next (llink))
      {
        GeglRect        dirt_rect;
        GeglConnection *connection = llink->data;
        GeglNode       *source_node = gegl_connection_get_source_node (connection);
        GeglPad        *dest_pad    = gegl_connection_get_sink_pad (connection);
        
        dirt_rect = source_node->dirt_rect;
        dirt_rect = gegl_operation_get_affected_region (operation, gegl_pad_get_name (dest_pad), dirt_rect);
        gegl_rect_bounding_box (&rect, &rect, &dirt_rect);
      }
  }

  node->dirt_rect = rect;
/*
  if (rect.w != 0 && rect.h != 0)
    g_warning ("visit dirt: %s %i,%i %ix%i",
       gegl_node_get_debug_name (node),
       rect.x, rect.y, rect.w, rect.h);
  */
  time = gegl_ticks () - time;
  gegl_instrument ("process", gegl_node_get_op_type_name (node), time);
  gegl_instrument (gegl_node_get_op_type_name (node), "dirt-compute", time);
}
