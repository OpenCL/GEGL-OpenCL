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
#include "interpolate-cubic.h"

#include "matrix.h"

#define CUBIC_ROW(dx, row, step) \
  transform_cubic(dx,\
                  (row)[0], (row)[step], (row)[step+step], (row)[step+step+step])

#define CUBIC_SCALED_ROW(dx, row, arow, step) \
  transform_cubic(dx, \
            (arow)[0]              * (row)[0], \
            (arow)[step]           * (row)[step], \
            (arow)[step+step]      * (row)[step+step], \
            (arow)[step+step+step] * (row)[step+step+step])

static inline gdouble
transform_cubic (gdouble    du,
                 gdouble    jm1,
                 gdouble    j,
                 gdouble    jp1,
                 gdouble    jp2)
{
  gdouble result;

  /* Catmull-Rom */
  result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * du +
               ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * du +
               ( - jm1 + jp1 ) ) * du + (j + j) ) / 2.0;

  return result;
}

void
affine_cubic (GeglBuffer *dest,
              GeglBuffer *src,
              Matrix3     matrix)
{
  gdouble  arecip;

  gint     x, y,
           i, j,
           pos,
           src_w    = src->width,
           src_h    = src->height,
           u,
           u1       = src->x,
           u2       = src_w - 1,
           pu,
           v,
           v1       = src->y,
           v2       = src_h - 1,
           pv;
  gfloat  *src_buf,
          *dest_buf,
          *src_ptr,
          *dest_ptr,
           abyss = 0.;
  gdouble  newval[4];
  gdouble  data[64];
  gdouble  du, dv, fu, fv;
  Matrix3  inverse;

  if (gegl_buffer_pixels (src) == 0 ||
      gegl_buffer_pixels (dest) == 0)
    return;
  src_buf  = g_new (gfloat, gegl_buffer_pixels (src) << 2);
  dest_buf = g_new (gfloat, gegl_buffer_pixels (dest) << 2);
  g_assert (src_buf && dest_buf);
  gegl_buffer_get (src, NULL, src_buf, babl_format ("RaGaBaA float"), 1.0);

  matrix3_copy (inverse, matrix);
  matrix3_invert (inverse);

  fu = du = inverse[0][0] * dest->x + inverse[0][1] * dest->y
            + inverse[0][2] - src->x;
  fv = dv = inverse[1][0] * dest->x + inverse[1][1] * dest->y
            + inverse[1][2] - src->y;

  for (dest_ptr = dest_buf, y = 0; y < dest->height; y++)
    {
      for (x = 0; x < dest->width; x++)
        {
          u = (gint) fu;
          v = (gint) fv;
          if (u <  v1 || v <  v1 || u >= u2 || v >= v2)
            {
              *dest_ptr++ = abyss;
              *dest_ptr++ = abyss;
              *dest_ptr++ = abyss;
              *dest_ptr++ = abyss;
            }
          else
            {
              /* get source values */
              for (pos=0, j = -2; j < 2; j++)
                 for (i = -2; i < 2; i++, pos+=4)
                   {
                      pu = CLAMP( u+i, u1, u2 );
                      pv = CLAMP( v+j, v1, v2 );
                      src_ptr = src_buf + ((pv * src_w + pu) << 2);
                      data[pos]   = src_ptr[0];
                      data[pos+1] = src_ptr[1];
                      data[pos+2] = src_ptr[2];
                      data[pos+3] = src_ptr[3];
                   }
               newval[3] = transform_cubic (fv-v,
                                            CUBIC_ROW(fu-u, data+3,    4),
                                            CUBIC_ROW(fu-u, data+3+16, 4),
                                            CUBIC_ROW(fu-u, data+3+32, 4),
                                            CUBIC_ROW(fu-u, data+3+48, 4));
               for (i = 0; i < 3; i++)
                 {
                   newval[i] = transform_cubic (fv-v,
                                                CUBIC_SCALED_ROW(fu-u, data+i,    data+3,  4),
                                                CUBIC_SCALED_ROW(fu-u, data+i+16, data+19, 4),
                                                CUBIC_SCALED_ROW(fu-u, data+i+32, data+35, 4),
                                                CUBIC_SCALED_ROW(fu-u, data+i+48, data+51, 4));
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
          fu += inverse [0][0];
          fv += inverse [1][0];
        }
      du += inverse [0][1];
      dv += inverse [1][1];
      fu = du;
      fv = dv;
    }
  gegl_buffer_set (dest, NULL, dest_buf, babl_format ("RaGaBaA float"));

  g_free (src_buf);
  g_free (dest_buf);
}

void
scale_cubic (GeglBuffer *dest,
             GeglBuffer *src,
             Matrix3     matrix)
{
  affine_cubic (dest, src, matrix);
}
