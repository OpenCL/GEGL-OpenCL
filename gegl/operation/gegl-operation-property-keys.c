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
 * Copyright 2014 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation.h"
#include "gegl-operations.h"
#include "gegl-operation-property-keys.h"

static GHashTable *
gegl_param_spec_get_property_key_ht (GParamSpec  *pspec,
                                     gboolean     create)
{
  GHashTable *ret = NULL;
  if (pspec)
    {
      GQuark quark = g_quark_from_static_string ("gegl-property-keys");
      ret = g_param_spec_get_qdata (pspec, quark);
      if (!ret && create)
      {
        ret = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
        g_param_spec_set_qdata_full (pspec, quark, ret, 
                                     (GDestroyNotify)g_hash_table_unref);
      }
    }
  return ret;
}

const gchar *
gegl_param_spec_get_property_key (GParamSpec  *pspec,
                                  const gchar *key_name)
{
  GHashTable *ht;
  ht = gegl_param_spec_get_property_key_ht (pspec, FALSE);
  if (ht)
    return g_hash_table_lookup (ht, key_name);
  return NULL;
}

void
gegl_param_spec_set_property_key (GParamSpec  *pspec,
                                  const gchar *key_name,
                                  const gchar *value)
{
  GHashTable *ht;
  ht = gegl_param_spec_get_property_key_ht (pspec, TRUE);
  g_hash_table_insert (ht, g_strdup (key_name), g_strdup (value));
}

static GHashTable *
gegl_operation_class_get_property_key_ht (GeglOperationClass *operation_class,
                                          const gchar        *property_name,
                                          gboolean            create)
{
  GParamSpec *pspec;
  pspec = g_object_class_find_property (G_OBJECT_CLASS (operation_class),
                                        property_name);
  if (pspec)
    return gegl_param_spec_get_property_key_ht (pspec, create);
  return NULL;
}

gchar **
gegl_operation_list_property_keys (const gchar *operation_name,
                                   const gchar *property_name,
                                   guint       *n_keys)
{
  GType         type;
  GObjectClass *klass;
  GHashTable   *ht = NULL;
  GList        *list, *l;
  gchar       **ret;
  int count;
  int i;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      if (n_keys)
        *n_keys = 0;
      return NULL;
    }
  klass  = g_type_class_ref (type);
  ht = gegl_operation_class_get_property_key_ht (GEGL_OPERATION_CLASS (klass),
                                                 property_name, FALSE);
  if (!ht)
  {
    count = 0;
    ret = g_malloc0 (sizeof (gpointer));;
  }
  else
  {
    count = g_hash_table_size (ht);
    ret = g_malloc0 (sizeof (gpointer) * (count + 1));
    list = g_hash_table_get_keys (ht);
    for (i = 0, l = list; l; l = l->next, i++)
      {
        ret[i] = l->data;
      }
    g_list_free (list);
  }
  if (n_keys)
    *n_keys = count;
  g_type_class_unref (klass);
  return ret;
}

void
gegl_operation_class_set_property_key (GeglOperationClass *operation_class,
                                       const gchar        *property_name,
                                       const gchar        *key_name,
                                       const gchar        *value)
{
  GParamSpec *pspec;
  pspec = g_object_class_find_property (G_OBJECT_CLASS (operation_class),
                                        property_name);
  if (pspec)
    gegl_param_spec_set_property_key (pspec, key_name, value);
}


const gchar *
gegl_operation_class_get_property_key (GeglOperationClass *operation_class,
                                       const gchar        *property_name,
                                       const gchar        *key_name)
{
  GParamSpec *pspec;
  pspec = g_object_class_find_property (G_OBJECT_CLASS (operation_class),
                                        property_name);
  if (pspec)
    return gegl_param_spec_get_property_key (pspec, key_name);
  return NULL;
}

const gchar *
gegl_operation_get_property_key (const gchar *operation_name,
                                 const gchar *property_name,
                                 const gchar *property_key_name)
{
  GType         type;
  GObjectClass *klass;
  const gchar  *ret = NULL;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      return NULL;
    }
  klass  = g_type_class_ref (type);
  ret = gegl_operation_class_get_property_key (GEGL_OPERATION_CLASS (klass), 
                                               property_name, 
                                               property_key_name);
  g_type_class_unref (klass);
  return ret;
}
