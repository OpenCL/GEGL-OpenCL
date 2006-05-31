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
#ifndef _TILE_CACHE_H
#define _TILE_CACHE_H

#include "gegl-buffer-types.h"
#include "gegl-tile-trait.h"

#define GEGL_TYPE_TILE_CACHE            (gegl_tile_cache_get_type ())
#define GEGL_TILE_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_CACHE, GeglTileCache))
#define GEGL_TILE_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_CACHE, GeglTileCacheClass))
#define GEGL_IS_TILE_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_CACHE))
#define GEGL_IS_TILE_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_CACHE))
#define GEGL_TILE_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_CACHE, GeglTileCacheClass))


typedef struct _GeglTileCache        GeglTileCache;
typedef struct _GeglTileCacheClass   GeglTileCacheClass;

struct _GeglTileCache
{
  GeglTileTrait parent_instance;
  GSList  *list;
  gint     size;
  gint     hits;
  gint     misses;

  gint     wash_percentage;
};

struct _GeglTileCacheClass
{
  GeglTileTraitClass parent_class;
};

GType       gegl_tile_cache_get_type            (void) G_GNUC_CONST;


#endif
