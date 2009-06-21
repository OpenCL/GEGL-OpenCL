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
 * Copyright 2003 Calvin Williamson
 *           2005-2008 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-operation.h"
#include "gegl-operations.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "gegl-operation-context.h"
#include "buffer/gegl-region.h"

static void
add_operations (GHashTable *hash,
                GType       parent)
{
  GType *types;
  guint  count;
  gint   no;

  types = g_type_children (parent, &count);
  if (!types)
    return;

  for (no = 0; no < count; no++)
    {
      GeglOperationClass *operation_class = g_type_class_ref (types[no]);
      if (operation_class->name)
        {
          g_hash_table_insert (hash, g_strdup (operation_class->name), (gpointer) types[no]);
        }
      if (operation_class->compat_name)
        {
          g_hash_table_insert (hash, g_strdup (operation_class->compat_name), (gpointer) types[no]);
        }
      add_operations (hash, types[no]);
    }
  g_free (types);
}

static GHashTable *gtype_hash = NULL;
GType
gegl_operation_gtype_from_name (const gchar *name)
{
  if (!gtype_hash)
    {
      gtype_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      add_operations (gtype_hash, GEGL_TYPE_OPERATION);
    }
  return (GType) g_hash_table_lookup (gtype_hash, name);
}

static GSList *operations_list = NULL;
static void addop (gpointer key,
                   gpointer value,
                   gpointer user_data)
{
  operations_list = g_slist_prepend (operations_list, key);
}

gchar **gegl_list_operations (guint *n_operations_p)
{
  gchar **pasp = NULL;
  gint    n_operations;
  gint    i;
  GSList *iter;
  gint    pasp_size = 0;
  gint    pasp_pos;

  if (!operations_list)
    {
      gegl_operation_gtype_from_name ("");
      g_hash_table_foreach (gtype_hash, addop, NULL);
      operations_list = g_slist_sort (operations_list, (GCompareFunc) strcmp);
    }

  n_operations = g_slist_length (operations_list);
  pasp_size   += (n_operations + 1) * sizeof (gchar *);
  for (iter = operations_list; iter != NULL; iter = g_slist_next (iter))
    {
      const gchar *name = iter->data;
      pasp_size += strlen (name) + 1;
    }
  pasp     = g_malloc (pasp_size);
  pasp_pos = (n_operations + 1) * sizeof (gchar *);
  for (iter = operations_list, i = 0; iter != NULL; iter = g_slist_next (iter), i++)
    {
      const gchar *name = iter->data;
      pasp[i] = ((gchar *) pasp) + pasp_pos;
      strcpy (pasp[i], name);
      pasp_pos += strlen (name) + 1;
    }
  pasp[i] = NULL;
  if (n_operations_p)
    *n_operations_p = n_operations;
  return pasp;
}

void
gegl_operation_gtype_cleanup (void)
{
  if (gtype_hash)
    {
      g_hash_table_destroy (gtype_hash);
      gtype_hash = NULL;
    }
}

/**
 * gegl_operation_set_need_rect:
 * @operation:
 * @context_id:
 * @input_pad_name:
 * @region:
 *
 * Calculates the need rect for the node feeding into @input_pad_name
 * for the given graph evaluation that is identified by
 * @context_id. The need rect of a node mathematically depends on the
 * ROI that is being evaluated and is the area that evaluation of the
 * ROI depends on for the node in question.
 **/
static void
gegl_operation_set_need_rect (GeglOperation       *operation,
                              gpointer             context_id,
                              const gchar         *input_pad_name,
                              const GeglRectangle *region)
{
  GeglNode     *child;         /* the node which need rect we are affecting */
  GeglRectangle child_need;    /* the need rect of the child */
  GeglPad      *output_pad;

  g_return_if_fail (GEGL_IS_OPERATION (operation));
  g_return_if_fail (GEGL_IS_NODE (operation->node));
  g_return_if_fail (input_pad_name != NULL);

  {
    GeglPad *input_pad = gegl_node_get_pad (operation->node, input_pad_name);
    if (!input_pad)
      return;
    output_pad = gegl_pad_get_connected_to (input_pad);
    if (!output_pad)
      return;
    child = gegl_pad_get_node (output_pad);
    if (!child)
      return;
  }

  {
    GeglOperationContext *child_context = gegl_node_get_context (child, context_id);
    gegl_rectangle_bounding_box (&child_need, &child_context->need_rect, region);
    gegl_rectangle_intersect (&child_need, &child->have_rect, &child_need);

      /* If we're cached */
      if (child->cache)
        {
          GeglCache *cache = child->cache;
          GeglRectangle valid_box;
          gegl_region_get_clipbox (cache->valid_region, &valid_box);

          if (child_need.width == 0  ||
              child_need.height == 0 ||
              gegl_region_rect_in (cache->valid_region, &child_need) == GEGL_OVERLAP_RECTANGLE_IN)
            {
              child_context->result_rect = child_need;
              child_context->cached = TRUE;
              child_need.width = 0;
              child_need.height = 0;
            }
        }

    gegl_node_set_need_rect (child, context_id, &child_need);
  }
}

gboolean
gegl_operation_calc_need_rects (GeglOperation *operation,
                                gpointer       context_id)
{
  GSList          *input_pads;
  GeglOperationContext *context;
  GeglRectangle    request;

  context = gegl_node_get_context (operation->node, context_id);
  request = context->need_rect;

  /* For each input, do get_required_for_output() then use
   * gegl_operation_set_need_rect()
   */
  for (input_pads = operation->node->input_pads;input_pads;input_pads=input_pads->next)
    {
      const gchar *pad_name = gegl_pad_get_name (input_pads->data);
      GeglRectangle rect;
      rect = gegl_operation_get_required_for_output (operation, pad_name, &request); 

      gegl_operation_set_need_rect (operation, context_id, pad_name, &rect);
    }
  return TRUE;
}
