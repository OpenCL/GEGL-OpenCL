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

#define GEGL_BUFFER_MAX_ITERATORS 6

#define GEGL_BUFFER_READ      1
#define GEGL_BUFFER_WRITE     2
#define GEGL_BUFFER_READWRITE (GEGL_BUFFER_READ|GEGL_BUFFER_WRITE)

typedef struct GeglBufferIterator
{
  gint          length;
  gpointer      data[GEGL_BUFFER_MAX_ITERATORS];
  GeglRectangle roi[GEGL_BUFFER_MAX_ITERATORS];
} GeglBufferIterator;


/**
 * gegl_buffer_iterator_new:
 * @buffer: a #GeglBuffer
 * @roi: the rectangle to iterate over
 * @format: the format we want to process this buffers data in, pass 0 to use the buffers format.
 * @flags: whether we need reading or writing to this buffer one of GEGL_BUFFER_READ, GEGL_BUFFER_WRITE and GEGL_BUFFER_READWRITE.
 *
 * Create a new buffer iterator, this buffer will be iterated through
 * in linear chunks, some chunks might be full tiles the coordinates, see
 * the documentation of gegl_buffer_iterator_next for how to use it and
 * destroy it.
 *
 * Returns: a new buffer iterator that can be used to iterate through the
 * buffers pixels.
 */
GeglBufferIterator * gegl_buffer_iterator_new  (GeglBuffer          *buffer,
                                                const GeglRectangle *roi, 
                                                const Babl          *format,
                                                guint                flags);

/**
 * gegl_buffer_iterator_add:
 * @iterator: a #GeglBufferIterator
 * @buffer: a #GeglBuffer
 * @roi: the rectangle to iterate over
 * @format: the format we want to process this buffers data in, pass 0 to use the buffers format.
 * @flags: whether we need reading or writing to this buffer.
 *
 * Adds an additional buffer iterator that will be processed in sync with
 * the original one, if the buffer doesn't align with the other for tile access
 * the corresponding scans and regions will be serialized automatically using
 * gegl_buffer_get.
 *
 * Returns: an integer handle refering to the indice in the iterator structure
 * of the added buffer.
 */
gint                 gegl_buffer_iterator_add  (GeglBufferIterator  *iterator,
                                                GeglBuffer          *buffer,
                                                const GeglRectangle *roi, 
                                                const Babl          *format,
                                                guint                flags);


/**
 * gegl_buffer_iterator_next:
 * @iterator: a #GeglBufferIterator
 *
 * Do an iteration, this causes a new set of iterator->data[] to become
 * available if there is more data to process. Changed data from a previous
 * iteration step will also be saved now. When there is no more data to
 * be processed NULL will be returned (and the iterator handle is no longer
 * valid).
 *
 * Returns: TRUE if there is more work FALSE if iteration is complete.
 */
gboolean             gegl_buffer_iterator_next (GeglBufferIterator *iterator);


#ifdef EXAMPLE

  GeglBufferIterator *gi;
  gi = gegl_buffer_iterator_new (buffer, roi,
                                 babl_format("Y' float"), GEGL_BUFFER_WRITE);
  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];
      gint    i;
      for (i=0; i<gi->length; i++)
        {
          buf[i]=0.5;
        }
    }


#endif

#endif
