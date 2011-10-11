/* This file is part of GEGL.
 * ck
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
 * Copyright 2006-2008 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_BUFFER_BACKEND_H__
#define __GEGL_BUFFER_BACKEND_H__

typedef struct _GeglTileSource            GeglTileSource;
typedef struct _GeglTileSourceClass       GeglTileSourceClass;

typedef struct _GeglTileBackend           GeglTileBackend;
typedef struct _GeglTileBackendClass      GeglTileBackendClass;
typedef struct _GeglTileBackendPrivate    GeglTileBackendPrivate;

typedef struct _GeglTile                  GeglTile;


typedef void   (*GeglTileCallback)       (GeglTile *tile,
                                          gpointer user_data);


#include "gegl-types.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-source.h"
#include "gegl-tile.h"

#endif
