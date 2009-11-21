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
 * Copyright 2006-2009 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_H__
#define __GEGL_TILE_H__

#include <glib-object.h>

#include "gegl-buffer-types.h"

/* the instance size of a GeglTile is a bit large, and should if possible be
 * trimmed down
 */
struct _GeglTile
{
 /* GObject          parent_instance;*/
  gint             ref_count;

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
#if ENABLE_MT
  GMutex          *mutex;
#endif

  /* the shared list is a doubly linked circular list */
  GeglTile        *next_shared;
  GeglTile        *prev_shared;

  void (*destroy_notify) (gpointer pixels,
                          gpointer data);
  gpointer         destroy_notify_data;
};

GeglTile   * gegl_tile_new        (gint     size);
GeglTile   * gegl_tile_new_bare   (void); /* special hack for linear bufs */
GeglTile   * gegl_tile_ref        (GeglTile *tile);
void         gegl_tile_unref      (GeglTile *tile);
void       * gegl_tile_get_format (GeglTile *tile);


/* lock a tile for writing, this would allow writing to buffers
 * later gotten with get_data()
 */
void         gegl_tile_lock       (GeglTile *tile);
/* get a pointer to the linear buffer of the tile.
 */
#define gegl_tile_get_data(tile)  ((guchar*)((tile)->data))

/* unlock the tile notifying the tile that we're done manipulating
 * the data.
 */
void         gegl_tile_unlock     (GeglTile *tile);



gboolean     gegl_tile_is_stored  (GeglTile *tile);
gboolean     gegl_tile_store      (GeglTile *tile);
void         gegl_tile_void       (GeglTile *tile);
GeglTile    *gegl_tile_dup        (GeglTile *tile);

/* computes the positive integer remainder (also for negative dividends)
 */
#define GEGL_REMAINDER(dividend, divisor) \
                   (((dividend) < 0) ? \
                    (divisor) - 1 - ((-((dividend) + 1)) % (divisor)) : \
                    (dividend) % (divisor))

#define gegl_tile_offset(coordinate, stride) GEGL_REMAINDER((coordinate), (stride))

/* helper function to compute tile indices and offsets for coordinates
 * based on a tile stride (tile_width or tile_height)
 */
#define gegl_tile_indice(coordinate,stride) \
  (((coordinate) >= 0)?\
      (coordinate) / (stride):\
      ((((coordinate) + 1) /(stride)) - 1))

/* utility low-level functions used by an undo system in horizon
 * where the geglbufer originated, kept around in case they
 * become useful again
 */
void         gegl_tile_swp        (GeglTile *a,
                                   GeglTile *b);
void         gegl_tile_cpy        (GeglTile *src,
                                   GeglTile *dst);

#endif
