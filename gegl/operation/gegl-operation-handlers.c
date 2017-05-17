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
 * Copyright 2016 Martin Blanchard <tchaik@gmx.com>
 *  */

#include "config.h"
#include <glib.h>

#include "gegl-operation-handlers.h"
#include "gegl-operation-handlers-private.h"

static GHashTable *load_handlers = NULL;
static GHashTable *save_handlers = NULL;

static gboolean
gegl_operation_handlers_register_util (GHashTable **handlers,
                                       const gchar *content_type,
                                       const gchar *handler)
{
  gchar *type = NULL;

  if (g_str_has_prefix (content_type, "."))
    type = g_utf8_casefold (content_type, -1);
  else if (g_str_has_prefix (content_type, "image/"))
    type = g_strdup (content_type);
  else
    return FALSE;

  if (*handlers == NULL)
    *handlers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  g_hash_table_insert (*handlers, type, g_strdup (handler));

  return TRUE;
}

gboolean
gegl_operation_handlers_register_loader (const gchar *content_type,
                                         const gchar *handler)
{
  return gegl_operation_handlers_register_util (&load_handlers,
                                                content_type,
                                                handler);
}

gboolean
gegl_operation_handlers_register_saver (const gchar *content_type,
                                        const gchar *handler)
{
  return gegl_operation_handlers_register_util (&save_handlers,
                                                content_type,
                                                handler);
}

static const gchar *
gegl_operation_handlers_get_util (GHashTable *handlers,
                                  const gchar *content_type,
                                  const gchar *handler_type,
                                  const gchar *fallback)
{
  const gchar *handler = NULL;
  gchar *type = NULL;

  if (handlers == NULL)
    return NULL;

  if (g_str_has_prefix (content_type, "."))
    type = g_utf8_casefold (content_type, -1);
  else if (g_str_has_prefix (content_type, "image/"))
    type = g_strdup (content_type);
  else
    return NULL;

  handler = g_hash_table_lookup (handlers, type);

  g_free (type);

  if (handler != NULL)
    return handler;

  g_warning ("No %s for content type \"%s\", falling back to \"%s\"",
             handler_type, content_type, fallback);

  return fallback;
}

const gchar *
gegl_operation_handlers_get_loader (const gchar *content_type)
{
  return gegl_operation_handlers_get_util (load_handlers,
                                           content_type,
                                           "loader",
                                           "gegl:magick-load");
}

const gchar *
gegl_operation_handlers_get_saver (const gchar *content_type)
{
  return gegl_operation_handlers_get_util (save_handlers,
                                           content_type,
                                           "saver",
                                           "gegl:png-save");
}

void
gegl_operation_handlers_cleanup (void)
{
  if (load_handlers != NULL)
    {
      g_hash_table_destroy (load_handlers);
      load_handlers = NULL;
    }

  if (save_handlers != NULL)
    {
      g_hash_table_destroy (save_handlers);
      save_handlers = NULL;
    }
}
