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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_STORAGE_H__
#define __GEGL_TILE_STORAGE_H__

#include "gegl-tile-handler-chain.h"
#include "gegl-tile-handler-cache.h"

/***
 * GeglTileStorage provide the command API to GeglBuffer, and setup a chain of GeglTileHandler to
 * treat and store tiles.
 */

#define GEGL_TYPE_TILE_STORAGE            (gegl_tile_storage_get_type ())
#define GEGL_TILE_STORAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_STORAGE, GeglTileStorage))
#define GEGL_TILE_STORAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_STORAGE, GeglTileStorageClass))
#define GEGL_IS_TILE_STORAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_STORAGE))
#define GEGL_IS_TILE_STORAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_STORAGE))
#define GEGL_TILE_STORAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_STORAGE, GeglTileStorageClass))

struct _GeglTileStorage
{
  GeglTileHandlerChain parent_instance;
  GeglTileHandlerCache *cache;
  GMutex        *mutex;
  const Babl    *format;
  gint           tile_width;
  gint           tile_height;
  gint           tile_size;
  gint           px_size;
  gint           width;
  gint           height;
  gchar         *path;
  gint           seen_zoom; /* the maximum zoom level we've seen tiles for */

  guint          idle_swapper;

  GeglTile      *hot_tile; /* cached tile for speeding up gegl_buffer_get_pixel
                              and gegl_buffer_set_pixel (1x1 sized gets/sets)*/
};

struct _GeglTileStorageClass
{
  GeglTileHandlerChainClass parent_class;
};

GType gegl_tile_storage_get_type (void) G_GNUC_CONST;

GeglTileStorage *
gegl_tile_storage_new (GeglTileBackend *backend);

#endif
