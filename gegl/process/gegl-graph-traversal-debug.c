/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Daniel Sabo
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"
#include "gegl.h"

#include "graph/gegl-node-private.h"
#include "graph/gegl-pad.h"

#include "process/gegl-graph-debug.h"
#include "process/gegl-graph-traversal.h"
#include "process/gegl-graph-traversal-private.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operation-context.h"
#include "operation/gegl-operation-context-private.h"

#include <stdio.h>

void
gegl_graph_dump_outputs (GeglNode *node)
{
  GeglGraphTraversal *path      = gegl_graph_build (node);
  GList              *list_iter = NULL;

  gegl_graph_prepare (path);

  for (list_iter = path->dfs_path; list_iter; list_iter = list_iter->next)
  {
    GeglNode *cur_node = GEGL_NODE (list_iter->data);
    if (gegl_node_get_pad (cur_node, "output"))
      {
        const Babl *format = gegl_operation_get_format (cur_node->operation, "output");
        printf ("%s: output=%s\n", gegl_node_get_debug_name (cur_node),
                                   format ? babl_get_name (format) : "N/A");
      }
    else
      {
        printf ("%s: sink\n", gegl_node_get_debug_name (cur_node));
      }

    if (cur_node->valid_have_rect)
      {
        printf ("  bounds: ");
        gegl_rectangle_dump (&cur_node->have_rect);
      }
  }

  gegl_graph_free (path);
}

void
gegl_graph_dump_request (GeglNode            *node,
                         const GeglRectangle *roi)
{
  GeglGraphTraversal *path      = gegl_graph_build (node);
  GList              *list_iter = NULL;

  gegl_graph_prepare (path);
  gegl_graph_prepare_request (path, roi, 0);

  for (list_iter = path->dfs_path; list_iter; list_iter = list_iter->next)
  {
    GeglNode *cur_node = GEGL_NODE (list_iter->data);
    GeglOperationContext *context = g_hash_table_lookup (path->contexts, cur_node);
    
    if (!context->cached)
      printf ("%s: result: ", gegl_node_get_debug_name (cur_node));
    else
      printf ("%s: result (cached): ", gegl_node_get_debug_name (cur_node));
    gegl_rectangle_dump (gegl_operation_context_get_need_rect (context));
  }

  gegl_graph_free (path);
}

