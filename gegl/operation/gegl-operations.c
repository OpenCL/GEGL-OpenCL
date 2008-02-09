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

#define GEGL_INTERNAL

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-operation.h"
#include "gegl-operations.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-node-context.h"

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

gboolean
gegl_operation_calc_source_regions (GeglOperation *operation,
                                    gpointer       context_id)
{
  GSList          *input_pads;
  GeglNodeContext *context;
  GeglRectangle    request;

  context = gegl_node_get_context (operation->node, context_id);
  request = context->need_rect;

  /* for each input, get_required_for_output use gegl_operation_set_source_region() */
  for (input_pads = operation->node->input_pads;input_pads;input_pads=input_pads->next)
    {
      const gchar *pad_name = gegl_pad_get_name (input_pads->data);
      GeglRectangle rect;
      rect = gegl_operation_get_required_for_output (operation, pad_name, &request); 

      gegl_operation_set_source_region (operation, context_id, pad_name, &rect);
    }
  return TRUE;
}

void
gegl_operation_vector_prop_changed (GeglVector    *vector,
                                    GeglOperation *operation)
{
  /* In the end forces a re-render, should be adapted to
   * allow a smaller region to be forced for re-rendering
   * when the vector is incrementally grown
   */
  g_object_notify (G_OBJECT (operation), "vector"); 
}
