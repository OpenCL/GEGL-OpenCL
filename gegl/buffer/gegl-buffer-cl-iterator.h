/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

#ifndef __GEGL_BUFFER_CL_ITERATOR_H__
#define __GEGL_BUFFER_CL_ITERATOR_H__

#include "gegl-buffer.h"
#include "opencl/gegl-cl.h"

#define GEGL_CL_BUFFER_MAX_ITERATORS 6

enum
{
  GEGL_CL_BUFFER_READ   = 1,
  GEGL_CL_BUFFER_WRITE  = 2,
  GEGL_CL_BUFFER_AUX    = 3
};

typedef struct GeglBufferClIterator
{
  size_t        size [GEGL_CL_BUFFER_MAX_ITERATORS];  /* length of current data in pixels */
  cl_mem        tex  [GEGL_CL_BUFFER_MAX_ITERATORS];
  GeglRectangle roi  [GEGL_CL_BUFFER_MAX_ITERATORS];
} GeglBufferClIterator;

gint gegl_buffer_cl_iterator_add (GeglBufferClIterator  *iterator,
                                  GeglBuffer            *buffer,
                                  const GeglRectangle   *roi,
                                  const Babl            *format,
                                  guint                  flags,
                                  GeglAbyssPolicy        abyss_policy);

gint gegl_buffer_cl_iterator_add_2 (GeglBufferClIterator  *iterator,
                                    GeglBuffer            *buffer,
                                    const GeglRectangle   *roi,
                                    const Babl            *format,
                                    guint                  flags,
                                    gint                   left,
                                    gint                   right,
                                    gint                   top,
                                    gint                   bottom,
                                    GeglAbyssPolicy        abyss_policy);

gint gegl_buffer_cl_iterator_add_aux (GeglBufferClIterator  *iterator,
                                      const GeglRectangle   *roi,
                                      const Babl            *format,
                                      gint                   left,
                                      gint                   right,
                                      gint                   top,
                                      gint                   bottom);

gboolean gegl_buffer_cl_iterator_next (GeglBufferClIterator *iterator, gboolean *err);

void gegl_buffer_cl_iterator_stop (GeglBufferClIterator *iterator);

GeglBufferClIterator *gegl_buffer_cl_iterator_new (GeglBuffer          *buffer,
                                                   const GeglRectangle *roi,
                                                   const Babl          *format,
                                                   guint                flags);
#endif
