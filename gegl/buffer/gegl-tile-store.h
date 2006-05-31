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
#ifndef _GEGL_TILE_STORE_H
#define _GEGL_TILE_STORE_H

#include <glib.h>
#include "gegl-buffer-types.h"
#include "gegl-tile.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_STORE            (gegl_tile_store_get_type ())
#define GEGL_TILE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_STORE, GeglTileStore))
#define GEGL_TILE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_STORE, GeglTileStoreClass))
#define GEGL_IS_TILE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_STORE))
#define GEGL_IS_TILE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_STORE))
#define GEGL_TILE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_STORE, GeglTileStoreClass))

typedef gint GeglTileMessage;

enum _GeglTileMessage
{
  GEGL_TILE_SET = 0,
  GEGL_TILE_IS_DIRTY,
  GEGL_TILE_IS_CACHED,
  GEGL_TILE_UNDO_START_GROUP,
  GEGL_TILE_ZOOM_UPDATE,
  GEGL_TILE_DIRTY,
  GEGL_TILE_FLUSH_DIRTY,
  GEGL_TILE_IDLE,
  GEGL_TILE_VOID,
  GEGL_TILE_LAST_MESSAGE
};

struct _GeglTileStore
{
  GObject       parent_instance;
};

struct _GeglTileStoreClass
{
  GObjectClass    parent_class;

  GeglTile     *(*get_tile) (GeglTileStore  *gegl_tile_store,
                             gint            x,
                             gint            y,
                             gint            z);

  gboolean      (*message)  (GeglTileStore  *gegl_tile_store,
                             GeglTileMessage message,
                             gint            x,
                             gint            y,
                             gint            z,
                             gpointer        data);
};

GType      gegl_tile_store_get_type (void) G_GNUC_CONST;

GeglTile * gegl_tile_store_get_tile (GeglTileStore *gegl_tile_store,
                                     gint           x,
                                     gint           y,
                                     gint           z);

gboolean   gegl_tile_store_message   (GeglTileStore   *gegl_tile_store,
                                      GeglTileMessage  message,
                                      gint             x,
                                      gint             y,
                                      gint             z,
                                      gpointer         data);

G_END_DECLS

#endif
