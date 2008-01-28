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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib.h>
#include "gegl-extension-handler.h"

static GHashTable *handlers = NULL;

void
gegl_extension_handler_register (const gchar *extension,
                                 const gchar *handler)
{
  if (!handlers)
    handlers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

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

void
gegl_extension_handler_cleanup (void)
{
  if (handlers)
    {
      g_hash_table_destroy (handlers);
      handlers = NULL;
    }
}
