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
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib.h>

static GHashTable *handlers = NULL;

void
gegl_extension_handler_register (const gchar *extension,
                                 const gchar *handler)
{
  const gchar *old;
  if (!handlers)
    handlers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  g_hash_table_insert (handlers, g_strdup (extension), g_strdup (handler));
}

const gchar *
gegl_extension_handler_get (const gchar *extension)
{
  const gchar *handler;
  if (!handlers)
    return NULL;
  handler = g_hash_table_lookup (handlers, extension);
  if (handler)
    return handler;
  return "magick-load";
}
