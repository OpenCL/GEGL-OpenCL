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
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-buffer-types.h"
#include "gegl-tile-handler.h"
#include "gegl-tile-handler-log.h"

G_DEFINE_TYPE (GeglTileHandlerLog, gegl_tile_handler_log, GEGL_TYPE_TILE_HANDLER)


static const char *commands[] =
{
  "idle",
  "set",
  "get",
  "is_cached",
  "exist",
  "-", /*void*/
  "flush",
  "refetch",
  "last command",
  "eeek",
  NULL
};

static gpointer
gegl_tile_handler_log_command (GeglTileSource  *gegl_tile_source,
                               GeglTileCommand  command,
                               gint             x,
                               gint             y,
                               gint             z,
                               gpointer         data)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (gegl_tile_source);
  gpointer         result = NULL;

  result = gegl_tile_handler_source_command (handler, command, x, y, z, data);

  switch (command)
    {
      case GEGL_TILE_IDLE:
        /* idle messages clutter logging output */
        break;
      default:
        if (result)
        g_print ("(%s %p %p %i·%i·%i ⇒%p)",
          commands[command], GINT_TO_POINTER(GPOINTER_TO_INT(gegl_tile_source)&0xffff), GINT_TO_POINTER(GPOINTER_TO_INT(data)&0xffff), x, y, z,
          result);
        else
        g_print ("(%s %p %p %i·%i·%i ☹)",
          commands[command], GINT_TO_POINTER(GPOINTER_TO_INT(gegl_tile_source)&0xffff), data, x, y, z);
    }
  return result;
}

static void
gegl_tile_handler_log_class_init (GeglTileHandlerLogClass *klass)
{
}

static void
gegl_tile_handler_log_init (GeglTileHandlerLog *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_handler_log_command;
}
