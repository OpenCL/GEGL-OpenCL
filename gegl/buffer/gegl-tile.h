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
#include "gegl-buffer-backend.h"

GeglTile   * gegl_tile_new            (gint             size);
GeglTile   * gegl_tile_new_bare       (void);
GeglTile   * gegl_tile_ref            (GeglTile         *tile);
void         gegl_tile_unref          (GeglTile         *tile);

/* lock a tile for writing, this would allow writing to buffers
 * later gotten with get_data()
 */
void         gegl_tile_lock           (GeglTile         *tile);

/* unlock the tile notifying the tile that we're done manipulating
 * the data.
 */
void         gegl_tile_unlock         (GeglTile         *tile);


void         gegl_tile_mark_as_stored (GeglTile         *tile);
gboolean     gegl_tile_is_stored      (GeglTile         *tile);
gboolean     gegl_tile_store          (GeglTile         *tile);
void         gegl_tile_void           (GeglTile         *tile);
GeglTile    *gegl_tile_dup            (GeglTile         *tile);

void         gegl_tile_set_rev        (GeglTile         *tile,
                                       guint             rev);
guint        gegl_tile_get_rev        (GeglTile         *tile);

guchar      *gegl_tile_get_data       (GeglTile         *tile);
void         gegl_tile_set_data       (GeglTile         *tile,
                                       gpointer          pixel_data,
                                       gint              pixel_data_size);
void         gegl_tile_set_data_full  (GeglTile         *tile,
                                       gpointer          pixel_data,
                                       gint              pixel_data_size,
                                       GDestroyNotify    destroy_notify,
                                       gpointer          destroy_notify_data);

void         gegl_tile_set_unlock_notify
                                      (GeglTile         *tile,
                                       GeglTileCallback  unlock_notify,
                                       gpointer          unlock_notify_data);


#endif
