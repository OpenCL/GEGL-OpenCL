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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_HANDLER_PRIVATE_H__
#define __GEGL_TILE_HANDLER_PRIVATE_H__

void   _gegl_tile_handler_set_tile_storage (GeglTileHandler      *handler,
                                            GeglTileStorage      *tile_storage);
void   _gegl_tile_handler_set_cache        (GeglTileHandler      *handler,
                                            GeglTileHandlerCache *cache);

GeglTileStorage      * _gegl_tile_handler_get_tile_storage (GeglTileHandler *handler);
GeglTileHandlerCache * _gegl_tile_handler_get_cache        (GeglTileHandler *handler);

#endif
