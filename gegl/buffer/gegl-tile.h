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
#ifndef GEGL_TILE_H
#define GEGL_TILE_H

#define GEGL_TYPE_TILE            (gegl_tile_get_type ())
#define GEGL_TILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE, GeglTile))
#define GEGL_TILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE, GeglTileClass))
#define GEGL_IS_TILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE))
#define GEGL_IS_TILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE))
#define GEGL_TILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE, GeglTileClass))
#include <glib-object.h>

#include "gegl-buffer-types.h"

/* the instance size of a GeglTile is a bit large, and should if possible be
 * trimmed down
 */
struct _GeglTile
{
  GObject        parent_object;
  guchar        *data;        /* A small linear buffer for pixel data */
  gint           size;        /* The size of the data element in bytes */

  GeglStorage   *storage;     /* the buffer from which this tile was retrieved
                               * needed for the tile to be able to store itself
                               * back when it is unreffed for the last time
                               */
  gint           storage_x, storage_y, storage_z;
  gint           x, y, z;


  guint          rev;         /* this tile revision */
  guint          stored_rev;  /* what revision was we when we from storage?
                                 (currently set to 1 when loaded from disk */

  guint          flags;       /* used to store zoom dirt info */

  gchar          lock;        /* number of times the tile is write locked
                               * should in theory just have the values 0/1
                               */

  /* the shared list is a doubly linked circular list */
  GeglTile      *next_shared;
  GeglTile      *prev_shared;
};

enum {
  GEGL_TILE_DIRT_TL = 1<<0,
  GEGL_TILE_DIRT_TR = 1<<1,
  GEGL_TILE_DIRT_BL = 1<<2,
  GEGL_TILE_DIRT_BR = 1<<3
};

struct _GeglTileClass
{
  GObjectClass   parent_class;
};

GType  gegl_tile_get_type (void) G_GNUC_CONST;

GeglTile   * gegl_tile_new             (gint         size);
GeglTile   * gegl_tile_new_from_data   (guchar      *data,
                                        gint         size);
guchar     * gegl_tile_get_data        (GeglTile    *tile);
void       * gegl_tile_get_format      (GeglTile    *tile);
void         gegl_tile_lock            (GeglTile    *tile);
void         gegl_tile_unlock          (GeglTile    *tile);
gboolean     gegl_tile_is_stored       (GeglTile    *tile);
gboolean     gegl_tile_store           (GeglTile    *tile);
void         gegl_tile_void            (GeglTile    *tile);
GeglTile    *gegl_tile_dup             (GeglTile    *tile);
/* utility low-level functions used by undo system */
void         gegl_tile_swp             (GeglTile    *a,
                                        GeglTile    *b);
void         gegl_tile_cpy             (GeglTile    *src,
                                        GeglTile    *dst);

#endif
