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

#ifndef __GEGL_BUFFER_TYPES_H__
#define __GEGL_BUFFER_TYPES_H__

#include "gegl-types.h"
#include "gegl-buffer-backend.h"


/* gegl-buffer-types.h is not installed, thus all of this is private to
 * GeglBuffer even though some of it leaks among the components of GeglBuffer
 * here... better than installing it in an installed header at least.
 */

struct _GeglTileBackendPrivate
{
  gint        tile_width;
  gint        tile_height;
  const Babl *format;    /* defaults to the babl format "R'G'B'A u8" */
  gint        px_size;   /* size of a single pixel in bytes */
  gint        tile_size; /* size of an entire tile in bytes */

  gboolean    flush_on_destroy;

  GeglRectangle extent;

  gpointer    storage;
  gboolean    shared;
};

typedef struct _GeglTileHandlerChain      GeglTileHandlerChain;

typedef struct _GeglTileStorage           GeglTileStorage;

typedef struct _GeglCache                 GeglCache;

typedef struct _GeglRegion                GeglRegion;

#endif
