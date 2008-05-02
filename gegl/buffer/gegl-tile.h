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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_H__
#define __GEGL_TILE_H__

#include <glib-object.h>

#include "gegl-buffer-types.h"

#define GEGL_TYPE_TILE            (gegl_tile_get_type ())
#define GEGL_TILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE, GeglTile))
#define GEGL_TILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE, GeglTileClass))
#define GEGL_IS_TILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE))
#define GEGL_IS_TILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE))
#define GEGL_TILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE, GeglTileClass))

/* the instance size of a GeglTile is a bit large, and should if possible be
 * trimmed down
 */
struct _GeglTile
{
  GObject          parent_instance;

  guchar          *data;        /* actual pixel data for tile, a linear buffer*/
  gint             size;        /* The size of the linear buffer */

  GeglTileStorage *tile_storage; /* the buffer from which this tile was
                                  * retrieved needed for the tile to be able to
                                  * store itself back (for instance when it is
                                  * unreffed for the last time)
                                  */
  gint             x, y, z;


  guint            rev;         /* this tile revision */
  guint            stored_rev;  /* what revision was we when we from tile_storage?
                                   (currently set to 1 when loaded from disk */

  gchar            lock;        /* number of times the tile is write locked
                                 * should in theory just have the values 0/1
                                 */
#if ENABLE_MP
  GMutex          *mutex;
#endif

  /* the shared list is a doubly linked circular list */
  GeglTile        *next_shared;
  GeglTile        *prev_shared;
};

struct _GeglTileClass
{
  GObjectClass  parent_class;
};

GType        gegl_tile_get_type   (void) G_GNUC_CONST;

GeglTile   * gegl_tile_new        (gint     size);
void       * gegl_tile_get_format (GeglTile *tile);


/* lock a tile for writing, this would allow writing to buffers
 * later gotten with get_data()
 */
void         gegl_tile_lock       (GeglTile *tile);
/* get a pointer to the linear buffer of the tile.
 */
guchar     * gegl_tile_get_data   (GeglTile *tile);
/* unlock the tile notifying the tile that we're done manipulating
 * the data.
 */
void         gegl_tile_unlock     (GeglTile *tile);



gboolean     gegl_tile_is_stored  (GeglTile *tile);
gboolean     gegl_tile_store      (GeglTile *tile);
void         gegl_tile_void       (GeglTile *tile);
GeglTile    *gegl_tile_dup        (GeglTile *tile);


/* helper functions to compute tile indices and offsets for coordinates
 * based on a tile stride (tile_width or tile_height)
 */
gint         gegl_tile_indice     (gint      coordinate,
                                   gint      stride);
gint         gegl_tile_offset     (gint      coordinate,
                                   gint      stride);

/* utility low-level functions used by undo system */
void         gegl_tile_swp        (GeglTile *a,
                                   GeglTile *b);
void         gegl_tile_cpy        (GeglTile *src,
                                   GeglTile *dst);

#endif
