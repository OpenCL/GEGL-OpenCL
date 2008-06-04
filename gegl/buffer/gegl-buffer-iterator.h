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
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_BUFFER_ITERATOR_H__
#define __GEGL_BUFFER_ITERATOR_H__

#include "gegl-buffer.h"
#include "gegl-buffer-private.h"

typedef struct GeglBufferTileIterator
{
  GeglBuffer    *buffer;
  GeglTile      *tile;
  GeglRectangle  roi;
  gint           col;
  gint           row;
  gint           real_col;
  gint           real_row;
  gboolean       write;
  GeglRectangle  subrect;
  gpointer       data;
  gpointer       sub_data;
  gint           rowstride;
} GeglBufferTileIterator;

typedef struct GeglBufferScanIterator {
  GeglBufferTileIterator tile_iterator;
  gint                   max_size;  /* in bytes */
  gint                   rowstride; /* allows computing  */
  gint                   length;
  gint                   row;
  gint                   real_row;
  gpointer               data;
  GeglRectangle          roi;
} GeglBufferScanIterator;


GeglBufferScanIterator *gegl_buffer_scan_iterator_new (GeglBuffer    *buffer,
                                                       GeglRectangle  roi,
                                                       gboolean       write);

void      gegl_buffer_scan_iterator_init (GeglBufferScanIterator *i,
                                          GeglBuffer             *buffer,
                                          GeglRectangle           roi,
                                          gboolean                write);
gboolean  gegl_buffer_scan_iterator_next (GeglBufferScanIterator *i);
gboolean  gegl_buffer_scan_compatible    (GeglBuffer *input,
                                          gint        x0,
                                          gint        y0,
                                          GeglBuffer *output,
                                          gint        x1,
                                          gint        y1);


#endif
