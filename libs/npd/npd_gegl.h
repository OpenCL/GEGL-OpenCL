/*
 * This file is part of N-point image deformation library.
 *
 * N-point image deformation library is free software: you can
 * redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * N-point image deformation library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with N-point image deformation library.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#ifndef __NPD_GEGL_H__
#define	__NPD_GEGL_H__

#include "npd_math.h"
#include "graphics.h"
#include <gegl.h>

struct _NPDImage
{
  gint     width;
  gint     height;
  NPDPoint position;
  guchar  *buffer;
};

void npd_gegl_set_pixel_color (NPDImage *image,
                               gint      x,
                               gint      y,
                               NPDColor *color);

void npd_gegl_get_pixel_color (NPDImage *image,
                               gint      x,
                               gint      y,
                               NPDColor *color);
void npd_gegl_create_image    (NPDImage   *image,
                               GeglBuffer *gegl_buffer,
                               const Babl *format);
void npd_gegl_destroy_image   (NPDImage *image);

#endif	/* __NPD_GEGL_H__ */
