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
 * Copyright 2006-2011 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_SOURCE_H__
#define __GEGL_TILE_SOURCE_H__

#include <glib-object.h>
#include <babl/babl.h>
#include "gegl-tile.h"

/***
 * GeglTileSource is the very top classes of the tile/buffer handling of Gegl. It defines the generic
 * command mechanism to interact with a set of tiles. This classe is derived in GeglTileBackend and
 * GeglTileHandler.
 */

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_SOURCE            (gegl_tile_source_get_type ())
#define GEGL_TILE_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_SOURCE, GeglTileSource))
#define GEGL_TILE_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_SOURCE, GeglTileSourceClass))
#define GEGL_IS_TILE_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_SOURCE))
#define GEGL_IS_TILE_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_SOURCE))
#define GEGL_TILE_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_SOURCE, GeglTileSourceClass))

typedef struct _GeglTileSourceClass GeglTileSourceClass;

struct _GeglTileSource
{
  GObject   parent_instance;
  gpointer  (*command)  (GeglTileSource  *gegl_tile_source,
                         GeglTileCommand  command,
                         gint             x,
                         gint             y,
                         gint             z,
                         gpointer         data);
  gpointer  padding[4];
};

struct _GeglTileSourceClass
{
  GObjectClass  parent_class;
  gpointer      padding[4];
};

GType      gegl_tile_source_get_type (void) G_GNUC_CONST;

static inline gpointer
gegl_tile_source_command (GeglTileSource  *source,
                          GeglTileCommand  command,
                          gint             x,
                          gint             y,
                          gint             z,
                          gpointer         data)
{
  return source->command (source, command, x, y, z, data);
}

/**
 * gegl_tile_source_get_tile:
 * @source: a GeglTileSource *
 * @x: x coordinate
 * @y: y coordinate
 * @z: tile zoom level
 *
 * Get a GeglTile *from the buffer, mipmap tiles for levels z!=0 will be
 * created on the fly as needed, empty tiles returned are copy on write
 * and must be locked before written to, and unlocked afterwards.
 *
 * Returns: the tile at x,y,z or NULL if the tile could not be provided.
 */
static inline GeglTile *
gegl_tile_source_get_tile (GeglTileSource *source,
                           gint            x,
                           gint            y,
                           gint            z)
{
  
  GeglTile *tile;
  

  tile = gegl_tile_source_command (source, GEGL_TILE_GET,
                                   x, y, z, NULL);

  return tile;
}


/**
 * gegl_buffer_get_tile: (skip)
 */

GeglTile *
gegl_buffer_get_tile (GeglBuffer *buffer,
                      gint        x,
                      gint        y,
                      gint        z);


/**
 * gegl_tile_source_set_tile:
 * @source: a GeglTileSource *
 * @x: x coordinate
 * @y: y coordinate
 * @z: tile zoom level
 * @tile: a #GeglTile
 *
 * Set a GeglTile in *from the buffer.
 *
 * Returns: the TRUE if the set was successful.
 */
static inline gboolean
gegl_tile_source_set_tile (GeglTileSource *source,
                           gint            x,
                           gint            y,
                           gint            z,
                           GeglTile       *tile)
{
  return gegl_tile_source_command (source, GEGL_TILE_SET, x, y, z, tile) != NULL;
}
/**
 * gegl_tile_source_is_cached:
 * @source: a GeglTileSource *
 * @x: tile x coordinate
 * @y: tile y coordinate
 * @z: tile zoom level
 *
 * Checks if a tile is in cache and easily retrieved.
 */
static inline gboolean
gegl_tile_source_is_cached (GeglTileSource *source,
                            gint            x,
                            gint            y,
                            gint            z)
{
  return gegl_tile_source_command (source, GEGL_TILE_IS_CACHED, x, y, z, NULL) != NULL;
}
/**
 * gegl_tile_source_exist:
 * @source: a GeglTileSource *
 * @x: x coordinate
 * @y: y coordinate
 * @z: tile zoom level
 *
 * Checks if a tile exists, this check would not cause the tile to be swapped
 * in.
 */
static inline gboolean
gegl_tile_source_exist (GeglTileSource *source,
                        gint            x,
                        gint            y,
                        gint            z)
{
  return gegl_tile_source_command (source, GEGL_TILE_EXIST, x, y, z, NULL) != NULL;
}

/**
 * gegl_tile_source_reinit:
 * @source: a GeglTileSource *
 *
 * Causes all tiles in cache to be refetched.
 */
static inline void
gegl_tile_source_reinit (GeglTileSource *source)
{
  gegl_tile_source_command (source, GEGL_TILE_REINIT, 0, 0, 0, NULL);
}

/**
 * gegl_tile_source_void:
 * @source: a GeglTileSource *
 * @x: x coordinate
 * @y: y coordinate
 * @z: tile zoom level
 *
 * Causes all references to a tile to be removed.
 */
static inline void
gegl_tile_source_void (GeglTileSource *source,
                       gint            x,
                       gint            y,
                       gint            z)
{
  gegl_tile_source_command (source, GEGL_TILE_VOID, x, y, z, NULL);
}

/*    INTERNAL API
 * gegl_tile_source_refetch:
 * @source: a GeglTileSource *
 * @x: x coordinate
 * @y: y coordinate
 * @z: tile zoom level
 *
 * A message used internally when watching external buffers to indicate that
 * a refresh of all data relating to the coordinates needs to be refetched.
 * Subsequent get calls should get new and valid data for the tile coordinates.
 */
static inline void
gegl_tile_source_refetch (GeglTileSource *source,
                          gint            x,
                          gint            y,
                          gint            z)
{
  gegl_tile_source_command (source, GEGL_TILE_REFETCH, x, y, z, NULL);
}
/*   INTERNAL API
 * gegl_tile_source_idle:
 * @source: a GeglTileSource *
 *
 * Allow different parts of the buffer to do idle work (saving cached
 * data lazily, perhaps prefetching in the future?), monitoring for
 * changes or other tasks. Used internally by the buffer object.
 *
 * Returns: the TRUE if some work was done.
 */
static inline gboolean
gegl_tile_source_idle (GeglTileSource *source)
{
  return gegl_tile_source_command (source, GEGL_TILE_IDLE, 0, 0, 0, NULL) != NULL;
}

G_END_DECLS

#endif
