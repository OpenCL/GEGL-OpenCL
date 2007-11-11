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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>

#include "gegl-buffer.h"

#ifndef _GEGL_BUFFER_SAVE_H
#define _GEGL_BUFFER_SAVE_H

typedef struct {
  gchar magic[16];
  gint  width, height, x, y;
  gchar format[32];
  guint tile_width, tile_height;
  guint bpp;
  gint  tile_count;

  guint padding1[12];
  guint padding[32];
} GeglBufferFileHeader;

typedef struct {
  gint  x;
  gint  y;
  gint  z;
  guint offset;
  guint flags;
} GeglTileEntry;

void gegl_buffer_save (GeglBuffer    *buffer,
                       const gchar   *path,
                       GeglRectangle *roi);

#endif
