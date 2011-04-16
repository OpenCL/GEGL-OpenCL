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

static GHashTable *load_handlers = NULL;
static GHashTable *save_handlers = NULL;

static void
gegl_extension_handler_register_util (GHashTable  **handlers,
                                      const gchar  *extension,
                                      const gchar  *handler)
{
  /* Case fold so we get case insensitive extension comparisions */
  gchar *ext = g_utf8_casefold (extension, -1);

  if (!*handlers)
    *handlers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  g_hash_table_insert (*handlers, ext, g_strdup (handler));
}

void
gegl_extension_handler_register (const gchar *extension,
                                 const gchar *handler)
{
  gegl_extension_handler_register_util (&load_handlers, extension, handler);
}

void
gegl_extension_handler_register_saver (const gchar *extension,
                                       const gchar *handler)
{
  gegl_extension_handler_register_util (&save_handlers, extension, handler);
}

static const gchar *
gegl_extension_handler_get_util (GHashTable *handlers,
                                 const gchar *extension,
                                 const gchar *handler_name,
                                 const gchar *def)
{
  const gchar *handler = NULL;
  gchar *ext = NULL;

  if (!handlers)
    return NULL;

  /* Case fold so we get case insensitive extension comparisions */
  ext = g_utf8_casefold (extension, -1);
  handler = g_hash_table_lookup (handlers, ext);
  g_free (ext);

  if (handler)
    return handler;

  g_warning ("No %s for extension \"%s\", falling back to \"%s\"",
             handler_name, extension, def);

  return def;
}

const gchar *
gegl_extension_handler_get (const gchar *extension)
{
  return gegl_extension_handler_get_util (load_handlers,
                                          extension,
                                          "loader",
                                          "gegl:magick-load");
}

const gchar *
gegl_extension_handler_get_saver (const gchar *extension)
{
  return gegl_extension_handler_get_util (save_handlers,
                                          extension,
                                          "saver",
                                          "gegl:png-save");
}

void
gegl_extension_handler_cleanup (void)
{
  if (load_handlers)
    {
      g_hash_table_destroy (load_handlers);
      load_handlers = NULL;
    }

  if (save_handlers)
    {
      g_hash_table_destroy (save_handlers);
      save_handlers = NULL;
    }
}
