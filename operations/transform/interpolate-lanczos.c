/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Geert Jordaens
 */

#include <string.h>
#include <math.h>
#include <gegl-plugin.h>
#include "interpolate-lanczos.h"

#include "matrix.h"

static gdouble *create_lanczos_lookup_transform (gint lanczos_width,
                                                 gint lanczos_spp);


static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < 0.0001)
    return 1.0;

  return sin (y) / y;
}

gdouble *
create_lanczos_lookup_transform (gint        lanczos_width,
                                 gint        lanczos_spp)
{
  const gint    lanczos_samples = (lanczos_spp * (lanczos_width + 1));
  const gdouble dx = (gdouble) lanczos_width / (gdouble) (lanczos_samples - 1);

  gdouble *lanczos = g_new (gdouble, lanczos_samples);
  gdouble  x       = 0.0;
  gint     i;

  for (i = 0; i < lanczos_samples; i++)
    {
      lanczos[i] = ((ABS (x) < lanczos_width) ?
                   (sinc (x) * sinc (x / lanczos_width)) : 0.0);
      x += dx;
    }

  return lanczos;
}

void
affine_lanczos (GeglBuffer *dest,
                GeglBuffer *src,
                Matrix3     matrix,
                gint        lanczos_width)
{
  gdouble *lanczos  = NULL;    /* Lanczos lookup table   */
  gdouble  x_sum, y_sum;       /* sum of Lanczos weights */
  gint     lanczos_width2;
  gint     lanczos_spp;

  gdouble x_kernel[lanczos_width*2+1],/* 1-D kernels of Lanczos window coeffs */
          y_kernel[lanczos_width*2+1];
  gdouble scale, arecip;




  gint    x, y,
          i, j,
          pos,
          src_w    = src->width,
          src_h    = src->height,
          u,
          u1       = src->x,
          u2       = src_w - 1,
          su, pu,
          v,
          v1       = src->y,
          v2       = src_h - 1,
          sv, pv;
  gfloat *src_buf,
         *dest_buf,
         *src_ptr,
         *dest_ptr,
          abyss = 0.;
  gdouble newval[4];
  gdouble du, dv, fu, fv;
  Matrix3 inverse;

  /*
   * parameter corretions
   * 1.   lanczos_width we do not settle for less than lanczos(3)
   * 2.   Window width is 2 times lanczos_width + 1
   * 3.   The minimum number of recalculate samples lanczos_spp is
   *      lanczos_width2.
   */
  lanczos_width  = (lanczos_width < 3) ? 3 : lanczos_width;
  lanczos_width2 = lanczos_width * 2 + 1;
  lanczos_spp    = lanczos_width2 * 10000;

  if (gegl_buffer_pixels (src) == 0 ||
      gegl_buffer_pixels (dest) == 0)
    return;
  src_buf  = g_new (gfloat, gegl_buffer_pixels (src) << 2);
  dest_buf = g_new (gfloat, gegl_buffer_pixels (dest) << 2);
  g_assert (src_buf && dest_buf);
  gegl_buffer_get (src, NULL, 1.0, babl_format ("RaGaBaA float"), src_buf);

  matrix3_copy (inverse, matrix);
  matrix3_invert (inverse);

  fu = du = inverse[0][0] * dest->x + inverse[0][1] * dest->y
            + inverse[0][2] - src->x;
  fv = dv = inverse[1][0] * dest->x + inverse[1][1] * dest->y
            + inverse[1][2] - src->y;

  /*
   * Allocate and fill lanczos lookup table
   */
  if (matrix [0][0] > 1.0 || matrix [0][1] > 1.0)
    {
       scale = MAX (matrix[0][0], matrix[0][1]) + 1;
       lanczos_spp *= (gint) scale;
    }
  lanczos = create_lanczos_lookup_transform (lanczos_width,
                                             lanczos_spp);

  for (dest_ptr = dest_buf, y = 0; y < dest->height; y++)
    {
      for (x = 0; x < dest->width; x++)
        {
          u = (gint) fu;
          v = (gint) fv;
          if (u < v1 || v < v1 || u >= u2 || v >= v2)
            {
              *((guint32 *) dest_ptr) = abyss;
              dest_ptr += 4;
            }
          else
            {
               /* get weight for fractional error */
               su = (gint) ((fu - u) * lanczos_spp);
               sv = (gint) ((fv - v) * lanczos_spp);
               /* fill 1D kernels */
               for (x_sum = y_sum = 0.0, i = lanczos_width; i >= -lanczos_width; i--)
                 {
                   pos = i * lanczos_spp;
                   x_sum += x_kernel[lanczos_width + i] = lanczos[ABS (su - pos)];
                   y_sum += y_kernel[lanczos_width + i] = lanczos[ABS (sv - pos)];
                 }

               /* normalise the weighted arrays */
               for (i = 0; i < lanczos_width2; i++)
                 {
                    x_kernel[i] /= x_sum;
                    y_kernel[i] /= y_sum;
                 }

              newval[0] = newval[1] = newval[2] = newval[3] = 0.0;
              for (j = 0; j < lanczos_width2; j++)
                 for (i = 0; i < lanczos_width2; i++)
                   {
                      pu = CLAMP( u+i-lanczos_width, u1, u2 );
                      pv = CLAMP( v+j-lanczos_width, v1, v2 );
                      src_ptr = src_buf + ((pv * src_w + pu) << 2);
                      newval[0] += y_kernel[j] * x_kernel[i] * src_ptr[0] * src_ptr[3];
                      newval[1] += y_kernel[j] * x_kernel[i] * src_ptr[1] * src_ptr[3];
                      newval[2] += y_kernel[j] * x_kernel[i] * src_ptr[2] * src_ptr[3];
                      newval[3] += y_kernel[j] * x_kernel[i] * src_ptr[3];
                   }
                   if (newval[3] <= 0.0)
                     {
                        arecip = 0.0;
                        newval[3] = 0;
                     }
                   else if (newval[3] > G_MAXDOUBLE)
                     {
                        arecip = 1.0 / newval[3];
                        newval[3] = G_MAXDOUBLE;
                     }
                   else
                     {
                         arecip = 1.0 / newval[3];
                     }

               *dest_ptr++ = CLAMP(newval[0] * arecip, 0, G_MAXDOUBLE);
               *dest_ptr++ = CLAMP(newval[1] * arecip, 0, G_MAXDOUBLE);
               *dest_ptr++ = CLAMP(newval[2] * arecip, 0, G_MAXDOUBLE);
               *dest_ptr++ = CLAMP(newval[3], 0, G_MAXDOUBLE);
            }
          fu += inverse[0][0];
          fv += inverse[1][0];
        }
      du += inverse[0][1];
      dv += inverse[1][1];
      fu = du;
      fv = dv;
    }
  gegl_buffer_set (dest, NULL, babl_format ("RaGaBaA float"), dest_buf);

  g_free (lanczos);
  g_free (src_buf);
  g_free (dest_buf);
}

void
scale_lanczos (GeglBuffer *dest,
               GeglBuffer *src,
               Matrix3     matrix,
               gint        lanczos_width)
{
  affine_lanczos (dest, src, matrix, lanczos_width);
}
