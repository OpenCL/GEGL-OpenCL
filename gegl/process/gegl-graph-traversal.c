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
 *           2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"
#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-instrument.h"

#include "buffer/gegl-region.h"

#include "graph/gegl-node-private.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "graph/gegl-connection.h"

#include "process/gegl-graph-traversal.h"
#include "process/gegl-graph-traversal-private.h"
#include "process/gegl-list-visitor.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operation-context.h"
#include "operation/gegl-operation-context-private.h"

typedef struct
{
  const gchar *name;
  GeglOperationContext *context;
} ContextConnection;

static void   free_context_connection                  (gpointer concon);
static GList *gegl_graph_get_connected_output_contexts (GeglGraphTraversal *path,
                                                        GeglPad            *output_pad);
static void   _gegl_graph_do_build                     (GeglGraphTraversal *path,
                                                        GeglNode           *node);
static GeglBuffer *gegl_graph_get_shared_empty         (GeglGraphTraversal *path);

static void
_gegl_graph_do_build (GeglGraphTraversal *path, GeglNode *node)
{
  GeglPad *pad = NULL;
  GeglListVisitor *list_visitor = g_object_new (GEGL_TYPE_LIST_VISITOR, NULL);

  /* We need to check the real node of the output/input pad in case this is a proxy node */
  pad = gegl_node_get_pad (node, "output");
  if (pad)
    {
      node = gegl_pad_get_node (pad);
    }
  else
    {
      pad = gegl_node_get_pad (node, "input");
      if (pad)
        node = gegl_pad_get_node (pad);
    }

  path->dfs_path = gegl_list_visitor_get_dfs_path (list_visitor, GEGL_VISITABLE (node));
  path->bfs_path = gegl_list_visitor_get_bfs_path (list_visitor, GEGL_VISITABLE (node));
  path->contexts = g_hash_table_new_full (NULL,
                                          NULL,
                                          NULL,
                                          (GDestroyNotify)gegl_operation_context_destroy);
  path->rects_dirty = FALSE;
  g_object_unref (list_visitor);
}

/**
 * gegl_graph_build:
 * @node: The node to build a traversal for
 * 
 * Build a traversal for @node.
 *
 * Return value: A new #GeglGraphTraversal
 */
GeglGraphTraversal *
gegl_graph_build (GeglNode *node)
{
  GeglGraphTraversal *result = g_new0 (GeglGraphTraversal, 1);

  _gegl_graph_do_build (result, node);

  return result;
}

/**
 * gegl_graph_rebuild:
 * @path: The traversal object to reuse
 * @node: The node to build a traversal for
 * 
 * Build a traversal for @node, reusing the scratch objects
 * from @path where possible.
 */
void
gegl_graph_rebuild (GeglGraphTraversal *path, GeglNode *node)
{
  g_list_free (path->dfs_path);
  g_list_free (path->bfs_path);
  g_hash_table_unref (path->contexts);

  /* Replaces everything but shared_empty */
  _gegl_graph_do_build (path, node);
}

void
gegl_graph_free (GeglGraphTraversal *path)
{
  g_list_free (path->dfs_path);
  g_list_free (path->bfs_path);
  g_hash_table_unref (path->contexts);
  if (path->shared_empty)
    g_object_unref (path->shared_empty);

  g_free (path);
}


/**
 * gegl_graph_get_bounding_box:
 * @path: The traversal path
 * 
 * Get output bounding box for this graph, which is the
 * have rect of the final node.
 *
 * Return value: Output rect of @path
 */
GeglRectangle
gegl_graph_get_bounding_box (GeglGraphTraversal *path)
{
  GeglNode *node = GEGL_NODE (g_list_last (path->dfs_path)->data);
  if (node->valid_have_rect)
    {
      return node->have_rect;
    }
  return *GEGL_RECTANGLE(0, 0, 0, 0);
}

/**
 * gegl_graph_prepare:
 * @path: The traversal path
 *
 * Prepare all nodes, initializing their output formats and have rects.
 */
void
gegl_graph_prepare (GeglGraphTraversal *path)
{
  GList *list_iter = NULL;

  for (list_iter = path->dfs_path; list_iter; list_iter = list_iter->next)
  {
    GeglNode *node = GEGL_NODE (list_iter->data);
    GeglNode *parent;
    GeglOperation *operation = node->operation;

    g_mutex_lock (&node->mutex);

    gegl_operation_prepare (operation);
    node->have_rect = gegl_operation_get_bounding_box (operation);
    node->valid_have_rect = TRUE;

    if (node->cache)
      {
        gegl_buffer_set_extent (GEGL_BUFFER (node->cache),
                                &node->have_rect);
      }

    g_mutex_unlock (&node->mutex);

    parent = gegl_node_get_parent (node);
    while (parent != NULL && parent->operation != NULL)
      {
        gegl_operation_prepare (parent->operation);
        parent = gegl_node_get_parent (parent);
      }

    if (!g_hash_table_contains (path->contexts, node))
      {
        GeglOperationContext *context = gegl_operation_context_new (node->operation);

        g_hash_table_insert (path->contexts,
                             node,
                             context);
      }
  }
}

/**
 * gegl_graph_prepare_request:
 * @path: The traversal path
 * @request_roi: The request rect
 *
 * Prepare the graph to render request_roi, this will calculate
 * the area that needs to be rendered from each node in the
 * graph to fulfill this request.
 */

void
gegl_graph_prepare_request (GeglGraphTraversal  *path,
                            const GeglRectangle *request_roi,
                            gint                 level)
{
  GList *list_iter = NULL;
  static const GeglRectangle empty_rect = {0, 0, 0, 0};

  g_return_if_fail (path->bfs_path);

  if (path->rects_dirty)
    {
      /* Zero all the needs rects so we can intersect with them below */
      for (list_iter = path->bfs_path; list_iter; list_iter = list_iter->next)
        {
          GeglNode *node = GEGL_NODE (list_iter->data);
          GeglOperationContext *context = g_hash_table_lookup (path->contexts, node);

          /* We only need to reset the need rect, result will always get overwritten */
          gegl_operation_context_set_need_rect (context, &empty_rect);

          /* Reset cached status, because the rect we need may have changed */
          context->cached = FALSE;
        }
    }

  path->rects_dirty = TRUE;

  {
    /* Prep the first node */
    GeglNode *node = GEGL_NODE (path->bfs_path->data);
    GeglOperationContext *context = g_hash_table_lookup (path->contexts, node);
    GeglRectangle new_need;

    g_return_if_fail (context);

    gegl_rectangle_intersect (&new_need, &node->have_rect, request_roi);

    gegl_operation_context_set_need_rect (context, &new_need);
    gegl_operation_context_set_result_rect (context, &new_need);
  }
  
  /* Iterate over all the nodes and propagate the requested rectangle */
  for (list_iter = path->bfs_path; list_iter; list_iter = list_iter->next)
    {
      GeglNode             *node      = GEGL_NODE (list_iter->data);
      GeglOperation        *operation = node->operation;
      GeglOperationContext *context;
      GeglRectangle        *request;
      GSList               *input_pads;
      
      context = g_hash_table_lookup (path->contexts, node);
      g_return_if_fail (context);
      
      request = gegl_operation_context_get_need_rect (context);

      if (request->width == 0 || request->height == 0)
        {
          gegl_operation_context_set_result_rect (context, &empty_rect);
          continue;
        }
      
      if (node->cache)
        {
          gint i;
          for (i = level; i >=0 && !context->cached; i--)
          {
            if (gegl_region_rect_in (node->cache->valid_region[level], request) == GEGL_OVERLAP_RECTANGLE_IN)
            {
              /* This node is cached and the cache fulfills our need rect */
              context->cached = TRUE;
              gegl_operation_context_set_result_rect (context, &empty_rect);
            }
          }
          if (context->cached)
            continue;
        }

      {
        /* Expand request if the operation has a minimum processing requirement */
        GeglRectangle full_request = gegl_operation_get_cached_region (operation, request);

        gegl_operation_context_set_need_rect (context, &full_request);

        /* FIXME: We could trim this down based on the cache, instead of being all or nothing */
        gegl_operation_context_set_result_rect (context, request);

        for (input_pads = node->input_pads; input_pads; input_pads = input_pads->next)
          {
            GeglPad *source_pad = gegl_pad_get_connected_to (input_pads->data);

            if (source_pad)
              {
                GeglNode             *source_node    = gegl_pad_get_node (source_pad);
                GeglOperationContext *source_context = g_hash_table_lookup (path->contexts, source_node);
                const gchar          *pad_name       = gegl_pad_get_name (input_pads->data);

                GeglRectangle rect, current_need, new_need;

                /* Combine this need rect with any existing request */
                rect = gegl_operation_get_required_for_output (operation, pad_name, &full_request);
                current_need = *gegl_operation_context_get_need_rect (source_context);

                gegl_rectangle_bounding_box (&new_need, &rect, &current_need);

                /* Limit request to the nodes output */
                gegl_rectangle_intersect (&new_need, &source_node->have_rect, &new_need);

                gegl_operation_context_set_need_rect (source_context, &new_need);
              }
          }
      }
    }
}

void
free_context_connection (gpointer concon)
{
  g_free (concon);
}

GList *
gegl_graph_get_connected_output_contexts (GeglGraphTraversal *path,
                                          GeglPad            *output_pad)
{
  GList *result = NULL;
  GSList *targets = gegl_pad_get_connections (output_pad);
  GSList *targets_iter;
  for (targets_iter = targets; targets_iter; targets_iter = g_slist_next (targets_iter))
    {
      GeglNode *target_node = gegl_connection_get_sink_node (targets_iter->data);
      GeglOperationContext *target_context = g_hash_table_lookup (path->contexts, target_node);
      
      /* Only include this target if it's part of the current path */
      if (target_context)
        {
          const gchar *target_pad_name = gegl_pad_get_name (gegl_connection_get_sink_pad (targets_iter->data));
          
          ContextConnection *result_element = g_new0 (ContextConnection, 1);
          result_element->name    = target_pad_name;
          result_element->context = target_context;
          
          result = g_list_prepend (result, result_element);
        }
    }
  return result;
}

GeglBuffer *
gegl_graph_get_shared_empty (GeglGraphTraversal *path)
{
  if (!path->shared_empty)
    {
      path->shared_empty = gegl_buffer_new_ram (GEGL_RECTANGLE (0, 0, 0, 0),
                                                gegl_babl_rgba_linear_float ());
      gegl_object_set_has_forked (G_OBJECT (path->shared_empty));
    }
  return path->shared_empty;
}


/**
 * gegl_graph_process:
 * @path: The traversal path
 *
 * Process the prepared request. This will return the
 * resulting buffer from the final node, or NULL if
 * that node is a sink.
 *
 * If gegl_graph_prepare_request has not been called
 * the behavior of this function is undefined.
 *
 * Return value: (transfer full): The result of the graph, or NULL if
 * there is no output pad.
 */
GeglBuffer *
gegl_graph_process (GeglGraphTraversal *path,
                    gint                level)
{
  GList *list_iter = NULL;
  GeglBuffer *result = NULL;
  GeglOperationContext *context = NULL;
  GeglOperationContext *last_context = NULL;
  GeglBuffer *operation_result = NULL;

  for (list_iter = path->dfs_path; list_iter; list_iter = list_iter->next)
    {
      GeglNode *node = GEGL_NODE (list_iter->data);
      GeglOperation *operation = node->operation;
      g_return_val_if_fail (node, NULL);
      g_return_val_if_fail (operation, NULL);
      
      GEGL_INSTRUMENT_START();

      operation_result = NULL;

      if (last_context)
        gegl_operation_context_purge (last_context);
      
      context = g_hash_table_lookup (path->contexts, node);
      g_return_val_if_fail (context, NULL);

      GEGL_NOTE (GEGL_DEBUG_PROCESS,
                 "Will process %s result_rect = %d, %d %d×%d",
                 gegl_node_get_debug_name (node),
                 context->result_rect.x, context->result_rect.y, context->result_rect.width, context->result_rect.height);
      
      if (context->need_rect.width > 0 && context->need_rect.height > 0)
        {
          if (context->cached)
            {
              GEGL_NOTE (GEGL_DEBUG_PROCESS,
                         "Using cached result for %s",
                         gegl_node_get_debug_name (node));
              operation_result = GEGL_BUFFER (node->cache);
            }
          else
            {
              /* provide something on input pad, always - this makes having
                 behavior depending on it not being set.. not work, is
                 sacrifising that worth it?
               */
              if (gegl_node_has_pad (node, "input") &&
                  !gegl_operation_context_get_object (context, "input"))
                {
                  gegl_operation_context_set_object (context, "input", G_OBJECT (gegl_graph_get_shared_empty(path)));
                }

              context->level = level;

              /* note: this hard-coding of "output" makes some more custom
               * graph topologies harder than neccesary.
               */
              gegl_operation_process (operation, context, "output", &context->need_rect, context->level);
              operation_result = GEGL_BUFFER (gegl_operation_context_get_object (context, "output"));

              if (operation_result && operation_result == (GeglBuffer *)operation->node->cache)
                gegl_cache_computed (operation->node->cache, &context->need_rect, level);
            }
        }
      else
        {
          operation_result = NULL;
        }

      if (operation_result)
        {
          GeglPad *output_pad = gegl_node_get_pad (node, "output");
          GList   *targets = gegl_graph_get_connected_output_contexts (path, output_pad);
          GList   *targets_iter;

          GEGL_NOTE (GEGL_DEBUG_PROCESS,
                     "Will deliver the results of %s:%s to %d targets",
                     gegl_node_get_debug_name (node),
                     "output",
                     g_list_length (targets));

          if (g_list_length (targets) > 1)
            gegl_object_set_has_forked (G_OBJECT (operation_result));

          for (targets_iter = targets; targets_iter; targets_iter = g_list_next (targets_iter))
            {
              ContextConnection *target_con = targets_iter->data;
              gegl_operation_context_set_object (target_con->context, target_con->name, G_OBJECT (operation_result));
            }
          g_list_free_full (targets, free_context_connection);
        }
      last_context = context;

      GEGL_INSTRUMENT_END ("process", gegl_node_get_operation (node));
    }
  if (last_context)
    {
      if (operation_result)
        result = g_object_ref (operation_result);
      else if (gegl_node_has_pad (last_context->operation->node, "output"))
        result = g_object_ref (gegl_graph_get_shared_empty (path));
      gegl_operation_context_purge (last_context);
    }

  return result;
}
