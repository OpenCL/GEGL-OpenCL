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


static GHashTable *gtype_hash = NULL;
G_LOCK_DEFINE_STATIC (gtype_hash);


static void
add_operations (GHashTable *hash,
                GType       parent)
{
  GType *types;
  guint  count;
  guint  no;

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

GType
gegl_operation_gtype_from_name (const gchar *name)
{
  GType ret = 0;

  /* FIXME: We only need mutual exclusion when we initialize. Maybe
   * move initialization to gegl_init()?
   */
  G_LOCK (gtype_hash);
  if (!gtype_hash)
    {
      gtype_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      add_operations (gtype_hash, GEGL_TYPE_OPERATION);
    }
  ret = (GType) g_hash_table_lookup (gtype_hash, name);
  G_UNLOCK (gtype_hash);

  return ret;
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

      G_LOCK (gtype_hash);
      g_hash_table_foreach (gtype_hash, addop, NULL);
      G_UNLOCK (gtype_hash);

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
  G_LOCK (gtype_hash);
  if (gtype_hash)
    {
      g_hash_table_destroy (gtype_hash);
      gtype_hash = NULL;
    }
  G_UNLOCK (gtype_hash);
}

gboolean gegl_can_do_inplace_processing (GeglOperation       *operation,
                                         GeglBuffer          *input,
                                         const GeglRectangle *result)
{
  if (!input ||
      GEGL_IS_CACHE (input))
    return FALSE;
  if (gegl_object_get_has_forked (input))
    return FALSE;

  if (input->format == gegl_operation_get_format (operation, "output") &&
      gegl_rectangle_contains (gegl_buffer_get_extent (input), result))
    return TRUE;
  return FALSE;
}
