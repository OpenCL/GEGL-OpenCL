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
 *           2013 Michael Henning
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


static GHashTable *gtype_hash        = NULL;
static GSList     *operations_list   = NULL;
static guint       gtype_hash_serial = 0;
G_LOCK_DEFINE_STATIC (gtype_hash);

void
gegl_operation_class_register_name (GeglOperationClass *klass,
                                    const gchar        *name)
{
  GType this_type, check_type;
  this_type = G_TYPE_FROM_CLASS (klass);

  G_LOCK (gtype_hash);

  /* FIXME: Maybe move initialization to gegl_init()? */
  if (!gtype_hash)
    {
      gtype_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    }

  check_type = (GType) g_hash_table_lookup (gtype_hash, name);
  if (check_type && check_type != this_type)
    {
      g_warning ("Adding %s shadows %s for operation %s",
                  g_type_name (this_type),
                  g_type_name (check_type),
                  name);
    }
  g_hash_table_insert (gtype_hash, g_strdup (name), (gpointer) this_type);

  if (!check_type)
    operations_list = g_slist_insert_sorted (operations_list, (gpointer) name,
                                             (GCompareFunc) strcmp);

  G_UNLOCK (gtype_hash);
}

static void
add_operations (GType parent)
{
  GType *types;
  guint  count;
  guint  no;
  G_LOCK_DEFINE_STATIC (type_class_peek_and_ref);

  types = g_type_children (parent, &count);
  if (!types)
    return;

  for (no = 0; no < count; no++)
    {
      G_LOCK (type_class_peek_and_ref);

      if (!g_type_class_peek (types[no]))
        g_type_class_ref (types[no]);

      G_UNLOCK (type_class_peek_and_ref);

      add_operations (types[no]);
    }
  g_free (types);
}

GType
gegl_operation_gtype_from_name (const gchar *name)
{
  /* If any new modules have been loaded, scan for GeglOperations */
  guint latest_serial;
  latest_serial = g_type_get_type_registration_serial ();
  if (gtype_hash_serial != latest_serial)
    {
      add_operations (GEGL_TYPE_OPERATION);
      gtype_hash_serial = latest_serial;
    }

  /* should only happen if no operations are found */
  if (!gtype_hash)
    return G_TYPE_INVALID;

  return (GType) g_hash_table_lookup (gtype_hash, name);
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

      /* should only happen if no operations are found */
      if (!operations_list)
        {
          if (n_operations_p)
            *n_operations_p = 0;
          return NULL;
        }
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
