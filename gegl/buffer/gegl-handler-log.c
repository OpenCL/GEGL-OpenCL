/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-handler.h"
#include "gegl-handler-log.h"

G_DEFINE_TYPE (GeglHandlerLog, gegl_handler_log, GEGL_TYPE_TILE_TRAIT)
static GObjectClass * parent_class = NULL;

static GeglTile *
get_tile (GeglTileStore *gegl_tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglTileStore *source = GEGL_HANDLER (gegl_tile_store)->source;
  GeglTile      *tile   = NULL;

  g_warning ("%p get_tile (%i,%i,%i)", (void *) gegl_tile_store, x, y, z);

  if (source)
    tile = gegl_tile_store_get_tile (source, x, y, z);

  return tile;
}

static char *messages[] =
{
  "set",         "is_dirty", "is_cached", "undo_start_group",                "zoom_update", "dirty",
  "flush_dirty", "idle",     "void",      "last_message (or added to enum)", "eekmsg",      "eekmsg"
};

static gboolean
message (GeglTileStore  *gegl_tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglHandler *handler = GEGL_HANDLER (gegl_tile_store);

  g_warning ("%p message(%s, x=%i, y=%i, z=%i, data=%p)", (void *) gegl_tile_store, messages[message], x, y, z, data);
  if (handler->source)
    return gegl_tile_store_message (handler->source, message, x, y, z, data);
  return FALSE;
}

static void
gegl_handler_log_class_init (GeglHandlerLogClass *klass)
{
  GeglTileStoreClass *gegl_tile_store_class = GEGL_TILE_STORE_CLASS (klass);

  gegl_tile_store_class->get_tile = get_tile;
  gegl_tile_store_class->message  = message;

  parent_class = g_type_class_peek_parent (klass);
}

static void
gegl_handler_log_init (GeglHandlerLog *self)
{
}
