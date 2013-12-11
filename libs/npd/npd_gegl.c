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

#include "npd_gegl.h"
#include <glib.h>
#include <gegl.h>

void
npd_compute_affinity (NPDPoint  *p11,
                      NPDPoint  *p21,
                      NPDPoint  *p31,
                      NPDPoint  *p12,
                      NPDPoint  *p22,
                      NPDPoint  *p32,
                      NPDMatrix *T)
{
  GeglMatrix3 Y, X;

  Y.coeff[0][0] = p12->x; Y.coeff[1][0] = p12->y; Y.coeff[2][0] = 1;
  Y.coeff[0][1] = p22->x; Y.coeff[1][1] = p22->y; Y.coeff[2][1] = 1;
  Y.coeff[0][2] = p32->x; Y.coeff[1][2] = p32->y; Y.coeff[2][2] = 1;

  X.coeff[0][0] = p11->x; X.coeff[1][0] = p11->y; X.coeff[2][0] = 1;
  X.coeff[0][1] = p21->x; X.coeff[1][1] = p21->y; X.coeff[2][1] = 1;
  X.coeff[0][2] = p31->x; X.coeff[1][2] = p31->y; X.coeff[2][2] = 1;

  gegl_matrix3_invert (&X);
  gegl_matrix3_multiply (&Y, &X, T);
}

void
npd_apply_transformation (NPDMatrix *T,
                          NPDPoint  *src,
                          NPDPoint  *dest)
{
  gdouble x = src->x, y = src->y;
  gegl_matrix3_transform_point (T, &x, &y);
  dest->x = x; dest->y = y;
}

void
npd_gegl_set_pixel_color (NPDImage *image,
                          gint      x,
                          gint      y,
                          NPDColor *color)
{
  if (x > -1 && x < image->width &&
      y > -1 && y < image->height)
    {
      gint position = 4 * (y * image->width + x);

      image->buffer[position + 0] = color->r;
      image->buffer[position + 1] = color->g;
      image->buffer[position + 2] = color->b;
      image->buffer[position + 3] = color->a;
    }
}

void
npd_gegl_get_pixel_color (NPDImage *image,
                          gint      x,
                          gint      y,
                          NPDColor *color)
{
  if (x > -1 && x < image->width &&
      y > -1 && y < image->height)
    {
      gint position = 4 * (y * image->width + x);

      color->r = image->buffer[position + 0];
      color->g = image->buffer[position + 1];
      color->b = image->buffer[position + 2];
      color->a = image->buffer[position + 3];
    }
  else
    {
      color->r = color->g = color->b = color->a = 0;
    }
}

void
npd_gegl_open_buffer (NPDImage *image)
{
  image->buffer = (guchar*) gegl_buffer_linear_open (image->gegl_buffer, NULL, &image->rowstride, image->format);
}

void
npd_gegl_close_buffer (NPDImage *image)
{
  gegl_buffer_linear_close (image->gegl_buffer, image->buffer);
}

void
npd_gegl_init_image (NPDImage   *image,
                     GeglBuffer *gegl_buffer,
                     const Babl *format)
{
  image->gegl_buffer = gegl_buffer;
  image->width = gegl_buffer_get_width (gegl_buffer);
  image->rowstride = image->width * babl_format_get_bytes_per_pixel (format);
  image->length = babl_format_get_n_components (format) * gegl_buffer_get_pixel_count (gegl_buffer);
  image->height = gegl_buffer_get_height (gegl_buffer);
  image->format = format;
  image->buffer = NULL;
  image->buffer_f = NULL;
}
