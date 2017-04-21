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
#include "gegl-debug.h"
#include "gegl-operation.h"
#include "gegl-operations.h"
#include "gegl-operation-context.h"

static gchar     **accepted_licenses       = NULL;
static GHashTable *known_operation_names   = NULL;
static GHashTable *visible_operation_names = NULL;
static GSList     *operations_list         = NULL;
static guint       gtype_hash_serial       = 0;

static GMutex operations_cache_mutex = { 0, };

void
gegl_operation_class_register_name (GeglOperationClass *klass,
                                    const gchar        *name,
                                    const gboolean      is_compat)
{
  GType this_type, check_type;
  this_type = G_TYPE_FROM_CLASS (klass);

  g_mutex_lock (&operations_cache_mutex);

  check_type = (GType) g_hash_table_lookup (known_operation_names, name);
  if (check_type && check_type != this_type)
    {
      g_warning ("Adding %s shadows %s for operation %s",
                  g_type_name (this_type),
                  g_type_name (check_type),
                  name);
    }

  g_hash_table_insert (known_operation_names, g_strdup (name), (gpointer) this_type);

  g_mutex_unlock (&operations_cache_mutex);
}

static void
add_operations (GType parent)
{
  GType *types;
  guint  count;
  guint  no;

  types = g_type_children (parent, &count);
  if (!types)
    return;

  for (no = 0; no < count; no++)
    {
      /*
       * Poke the operation so it registers its name with
       * gegl_operation_class_register_name
       */
      g_type_class_unref (g_type_class_ref (types[no]));

      add_operations (types[no]);
    }
  g_free (types);
}

static gboolean
gegl_operations_check_license (const gchar *operation_license)
{
  gint i;

  if (!accepted_licenses || !accepted_licenses[0])
    return FALSE;

  if (0 == g_ascii_strcasecmp (operation_license, "GPL1+"))
    {
      /* Search for GPL1 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL1", accepted_licenses[i]))
          return TRUE;
      /* Search for GPL2 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL2", accepted_licenses[i]))
          return TRUE;
      /* Search for GPL3 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL3", accepted_licenses[i]))
          return TRUE;
    }
  else if (0 == g_ascii_strcasecmp (operation_license, "GPL2+"))
    {
      /* Search for GPL2 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL2", accepted_licenses[i]))
          return TRUE;
      /* Search for GPL3 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL3", accepted_licenses[i]))
          return TRUE;
    }
  else if (0 == g_ascii_strcasecmp (operation_license, "GPL3+"))
    {
      /* Search for GPL3 */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp ("GPL3", accepted_licenses[i]))
          return TRUE;
    }
  else
    {
      /* Search for exact match */
      for (i = 0; accepted_licenses[i]; ++i)
        if (0 == g_ascii_strcasecmp (operation_license, accepted_licenses[i]))
          return TRUE;
    }

  return FALSE;
}

static void
gegl_operations_update_visible (void)
{
  GHashTableIter iter;
  const gchar   *iter_key;
  GType          iter_value;

  g_mutex_lock (&operations_cache_mutex);

  g_hash_table_remove_all (visible_operation_names);
  g_slist_free (operations_list);

  operations_list = NULL;

  g_hash_table_iter_init (&iter, known_operation_names);

  while (g_hash_table_iter_next (&iter, (gpointer)&iter_key, (gpointer)&iter_value))
    {
      GObjectClass       *object_class;
      GeglOperationClass *operation_class;
      const gchar        *operation_license;

      object_class = g_type_class_ref (iter_value);
      operation_class = GEGL_OPERATION_CLASS (object_class);

      operation_license = g_hash_table_lookup (operation_class->keys, "license");

      if (!operation_license || gegl_operations_check_license (operation_license))
        {
          if (operation_license)
            {
              GEGL_NOTE (GEGL_DEBUG_LICENSE, "Accepted %s for %s", operation_license, iter_key);
            }

          if (operation_class->name && (0 == strcmp (iter_key, operation_class->name)))
            {
              /* Is the primary name of the operation */
              operations_list = g_slist_insert_sorted (operations_list, (gpointer) iter_key,
                                                       (GCompareFunc) strcmp);
            }

          g_hash_table_insert (visible_operation_names, g_strdup (iter_key), (gpointer) iter_value);
        }
      else if (operation_license)
        {
          GEGL_NOTE (GEGL_DEBUG_LICENSE, "Rejected %s for %s", operation_license, iter_key);
        }

      g_type_class_unref (object_class);
    }

  g_mutex_unlock (&operations_cache_mutex);
}

void
gegl_operations_set_licenses_from_string (const gchar *license_str)
{
  g_mutex_lock (&operations_cache_mutex);

  if (accepted_licenses)
    g_strfreev (accepted_licenses);

  accepted_licenses = g_strsplit (license_str, ",", 0);

  g_mutex_unlock (&operations_cache_mutex);

  gegl_operations_update_visible ();
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

      gegl_operations_update_visible ();
    }

  return (GType) g_hash_table_lookup (visible_operation_names, name);
}

gboolean
gegl_has_operation (const gchar *operation_type)
{
  return gegl_operation_gtype_from_name (operation_type) != 0;
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

  g_mutex_lock (&operations_cache_mutex);

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

  g_mutex_unlock (&operations_cache_mutex);

  return pasp;
}

void
gegl_operation_gtype_init (void)
{
  g_mutex_lock (&operations_cache_mutex);

  if (!known_operation_names)
    known_operation_names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  if (!visible_operation_names)
    visible_operation_names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_mutex_unlock (&operations_cache_mutex);
}

void
gegl_operation_gtype_cleanup (void)
{
  g_mutex_lock (&operations_cache_mutex);
  if (known_operation_names)
    {
      g_hash_table_destroy (known_operation_names);
      known_operation_names = NULL;

      g_hash_table_destroy (visible_operation_names);
      visible_operation_names = NULL;

      g_slist_free (operations_list);
      operations_list = NULL;
    }
  g_mutex_unlock (&operations_cache_mutex);
}

gboolean
gegl_can_do_inplace_processing (GeglOperation       *operation,
                                GeglBuffer          *input,
                                const GeglRectangle *result)
{
  if (!input)
    return FALSE;
  if (gegl_object_get_has_forked (G_OBJECT (input)))
    return FALSE;

  if (gegl_buffer_get_format (input) == gegl_operation_get_format (operation, "output") &&
      gegl_rectangle_contains (gegl_buffer_get_extent (input), result))
    return TRUE;
  return FALSE;
}

static GQuark
gegl_has_forked_quark (void)
{
  static GQuark the_quark = 0;

  if (G_UNLIKELY (the_quark == 0))
    the_quark = g_quark_from_static_string ("gegl-object-has-forked");

  return the_quark;
}

void
gegl_object_set_has_forked (GObject *object)
{
  g_object_set_qdata (object, gegl_has_forked_quark (), (void*)0xf);
}


gboolean
gegl_object_get_has_forked (GObject *object)
{
  return g_object_get_qdata (object, gegl_has_forked_quark ()) != NULL;
}
