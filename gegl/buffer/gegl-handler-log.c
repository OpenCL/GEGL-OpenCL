/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-handler.h"
#include "gegl-handler-log.h"

G_DEFINE_TYPE (GeglHandlerLog, gegl_handler_log, GEGL_TYPE_HANDLER)

static GeglTile *
get_tile (GeglProvider *gegl_provider,
          gint          x,
          gint          y,
          gint          z)
{
  GeglProvider *provider = GEGL_HANDLER (gegl_provider)->provider;
  GeglTile      *tile   = NULL;

  g_print ("(get %p %i,%i,%i)", (void *) gegl_provider, x, y, z);

  if (provider)
    tile = gegl_provider_get_tile (provider, x, y, z);

  return tile;
}

static char *messages[] =
{
  "idle", "set",  "is_cached", "exist", "void", "void tl", "void tr", "void bl",
  "void br", "undo start group", "last message", "eeek", NULL
};

static gboolean
message (GeglProvider  *gegl_provider,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglHandler *handler = GEGL_HANDLER (gegl_provider);
  gboolean     result = FALSE;

  result = gegl_handler_chain_up (handler, message, x, y, z, data);

  switch (message)
    {
      case GEGL_TILE_IDLE:
        break;
      default:
        g_print ("(%s %p %p %i,%i,%i => %s)", 
          messages[message], (void *) gegl_provider, data, x, y, z,
          result?"1":"0");
    }
  return result;
}

static void
gegl_handler_log_class_init (GeglHandlerLogClass *klass)
{
  GeglProviderClass *provider_class = GEGL_PROVIDER_CLASS (klass);

  provider_class->get_tile = get_tile;
  provider_class->message  = message;
}

static void
gegl_handler_log_init (GeglHandlerLog *self)
{
}
