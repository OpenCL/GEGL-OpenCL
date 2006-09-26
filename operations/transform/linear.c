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
 * Copyright 2006 Philip Lafleur
 */

#include <string.h>
#include <math.h>
#include <gegl-plugin.h>

#include "matrix.h"

void
affine_linear (GeglBuffer *dest,
               GeglBuffer *src,
               Matrix3     matrix,
               gboolean    hard_edges)
{
  gint     x, y,
           src_w    = src->width,
           src_w_m1 = src_w - 1,
           src_h    = src->height,
           src_h_m1 = src_h - 1,
           src_rowstride = src_w << 2;
  guint8  *src_buf,
          *dest_buf,
          *src_ptr,
          *dest_ptr;
  guint32  abyss = 0;
  Matrix3  inverse;
  gdouble  u_start,
           v_start;

  if (gegl_buffer_pixels (src) == 0 ||
      gegl_buffer_pixels (dest) == 0)
    return;
  src_buf  = g_malloc (gegl_buffer_pixels (src) << 2);
  dest_buf = g_malloc (gegl_buffer_pixels (dest) << 2);
  g_assert (src_buf && dest_buf);
  gegl_buffer_get_fmt (src, src_buf, babl_format ("RGBA u8"));

  /* expand borders */
  /* bottom row */
  memcpy (src_buf + src_h_m1 * src_rowstride,
          src_buf + (src_h - 2) * src_rowstride,
          src_rowstride);

  /* right column */
  src_ptr = src_buf + (src_w_m1 << 2);
  for (y = src_h; y--; src_ptr += src_rowstride)
    *((guint32 *) src_ptr) = *((guint32 *) (src_ptr - 4));

  if (! hard_edges)
    {
      /* top row */
      memcpy (src_buf, src_buf + src_rowstride, src_rowstride);

      /* left column */
      src_ptr = src_buf;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *((guint32 *) src_ptr) = *((guint32 *) (src_ptr + 4));

      /* make expanded borders transparent */
      /* left column */
      src_ptr = src_buf + 3;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *src_ptr = 0;

      /* right column */
      src_ptr = src_buf + (src_w_m1 << 2) + 3;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *src_ptr = 0;

      /* top row */
      src_ptr = src_buf + 3;
      for (x = src_w; x--; src_ptr += 4)
        *src_ptr = 0;

      /* bottom row */
      src_ptr = src_buf + ((src_h_m1 * src_w) << 2) + 3;
      for (x = src_w; x--; src_ptr += 4)
        *src_ptr = 0;
    }

  matrix3_copy (inverse, matrix);
  matrix3_invert (inverse);

  u_start = inverse[0][0] * dest->x + inverse[0][1] * dest->y
            + inverse[0][2] - src->x;
  v_start = inverse[1][0] * dest->x + inverse[1][1] * dest->y
            + inverse[1][2] - src->y;

  for (dest_ptr = dest_buf, y = dest->height; y--;)
    {
      gdouble u_float = u_start,
              v_float = v_start;

      for (x = dest->width; x--;)
        {
          gint u = u_float,
               v = v_float;

          if (G_LIKELY (u >= 0 &&
                        v >= 0 &&
                        u < src_w_m1 &&
                        v < src_h_m1))
            {
              gdouble fu, fv,
                      fu_inv, fv_inv,
                      factor0,
                      factor1,
                      factor2,
                      factor3,
                      r, g, b, a;

              fu = u_float - u;
              fu = MAX (fu, 0.);
              fu_inv = 1. - fu;

              fv = v_float - v;
              fv = MAX (fv, 0.);
              fv_inv = 1. - fv;

              factor0 = fv_inv * fu_inv;
              factor1 = fv_inv * fu;
              factor2 = fv * fu_inv;
              factor3 = fv * fu;

              src_ptr = src_buf + ((v * src_w + u) << 2);
              r = src_ptr [0] * factor0;
              g = src_ptr [1] * factor0;
              b = src_ptr [2] * factor0;
              a = src_ptr [3] * factor0;

              r += src_ptr [4] * factor1;
              g += src_ptr [5] * factor1;
              b += src_ptr [6] * factor1;
              a += src_ptr [7] * factor1;

              src_ptr += src_rowstride;
              r += src_ptr [0] * factor2;
              g += src_ptr [1] * factor2;
              b += src_ptr [2] * factor2;
              a += src_ptr [3] * factor2;

              r += src_ptr [4] * factor3;
              g += src_ptr [5] * factor3;
              b += src_ptr [6] * factor3;
              a += src_ptr [7] * factor3;

              *dest_ptr++ = r;
              *dest_ptr++ = g;
              *dest_ptr++ = b;
              *dest_ptr++ = a;
            }
          else
            {
              *((guint32 *) dest_ptr) = abyss;
              dest_ptr += 4;
            }

          u_float += inverse [0][0];
          v_float += inverse [1][0];
        }

      u_start += inverse [0][1];
      v_start += inverse [1][1];
    }

  gegl_buffer_set_fmt (dest, dest_buf, babl_format ("RGBA u8"));

  g_free (src_buf);
  g_free (dest_buf);
}

void
scale_linear (GeglBuffer *dest,
              GeglBuffer *src,
              Matrix3     matrix,
              gboolean    hard_edges)
{
  gint     x, y,
           src_w          = src->width,
           src_w_m1       = src_w - 1,
           src_h          = src->height,
           src_h_m1       = src_h - 1,
           dest_w         = dest->width,
           dest_h         = dest->height,
           src_rowstride  = src_w << 2,
           dest_rowstride = dest_w << 2,
           skip_start     = 0,
           skip_left      = 0,
           skip_right     = 0,
           skip_end       = 0;
  guint8  *src_buf,
          *dest_buf,
          *src_start,
          *dest_start,
          *src_ptr,
          *dest_ptr;
  Matrix3  inverse;
  gdouble  u_start,
           v_start,
           u_float,
           v_float;

  if (gegl_buffer_pixels (src) == 0 ||
      gegl_buffer_pixels (dest) == 0)
    return;
  src_buf  = g_malloc (gegl_buffer_pixels (src) << 2);
  dest_buf = g_malloc (gegl_buffer_pixels (dest) << 2);
  g_assert (src_buf && dest_buf);
  gegl_buffer_get_fmt (src, src_buf, babl_format ("RGBA u8"));

  /* expand borders */
  /* bottom row */
  memcpy (src_buf + src_h_m1 * src_rowstride,
          src_buf + (src_h - 2) * src_rowstride,
          src_rowstride);

  /* right column */
  src_ptr = src_buf + (src_w_m1 << 2);
  for (y = src_h; y--; src_ptr += src_rowstride)
    *((guint32 *) src_ptr) = *((guint32 *) (src_ptr - 4));

  if (! hard_edges)
    {
      /* top row */
      memcpy (src_buf, src_buf + src_rowstride, src_rowstride);

      /* left column */
      src_ptr = src_buf;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *((guint32 *) src_ptr) = *((guint32 *) (src_ptr + 4));

      /* make expanded borders transparent */
      /* left column */
      src_ptr = src_buf + 3;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *src_ptr = 0;

      /* right column */
      src_ptr = src_buf + (src_w_m1 << 2) + 3;
      for (y = src_h; y--; src_ptr += src_rowstride)
        *src_ptr = 0;

      /* top row */
      src_ptr = src_buf + 3;
      for (x = src_w; x--; src_ptr += 4)
        *src_ptr = 0;

      /* bottom row */
      src_ptr = src_buf + ((src_h_m1 * src_w) << 2) + 3;
      for (x = src_w; x--; src_ptr += 4)
        *src_ptr = 0;
    }

  matrix3_copy (inverse, matrix);
  matrix3_invert (inverse);

  u_start = inverse[0][0] * dest->x + inverse[0][1] * dest->y
            + inverse[0][2] - src->x;
  v_start = inverse[1][0] * dest->x + inverse[1][1] * dest->y
            + inverse[1][2] - src->y;

  /* clear area outside source data
   * (due to filtering and rounding errors in rect calculation) */
  dest_ptr = dest_buf + (dest_h - 1) * dest_rowstride;
  v_float = v_start + (dest_h - 1) * inverse [1][1];
  for (; ((gint) v_float < 0 || (gint) v_float >= src_h_m1) && dest_h; dest_h--)
    {
      memset (dest_ptr, 0, dest_rowstride);
      dest_ptr -= dest_rowstride;

      skip_end += dest_rowstride;
      v_float -= inverse [1][1];
    }

  dest_ptr = dest_buf;
  for (; ((gint) v_start < 0 || (gint) v_start >= src_h_m1) && dest_h; dest_h--)
    {
      memset (dest_ptr, 0, dest_rowstride);
      dest_ptr += dest_rowstride;

      skip_start += dest_rowstride;
      v_start += inverse [1][1];
    }

  u_float = u_start + (dest_w - 1) * inverse [0][0];
  for (; ((gint) u_float < 0 || (gint) u_float >= src_w_m1) && dest_w; dest_w--)
    {
      skip_right += 4;
      u_float -= inverse [0][0];
    }
  dest_ptr += dest_rowstride - skip_right;
  for (y = dest_h; y--; dest_ptr += dest_rowstride)
    memset (dest_ptr, 0, skip_right);

  for (; ((gint) u_start < 0 || (gint) u_start >= src_w_m1) && dest_w; dest_w--)
    {
      skip_left += 4;
      u_start += inverse [0][0];
    }
  dest_ptr = dest_buf + skip_start;
  for (y = dest_h; y--; dest_ptr += dest_rowstride)
    memset (dest_ptr, 0, skip_left);

  dest_ptr = dest_start = dest_buf + skip_start + skip_left;

  /* if scale factor is > .5, some results can be saved for
   * use in subsequent calculations (either vertically or horizontally,
   * depending on which scale factor is greater) */
  if (fabs (matrix [0][0]) > .5 &&
      fabs (matrix [0][0]) >= fabs (matrix [1][1]))
    for (y = dest_h; y--; v_start += inverse [1][1])
      {
        /*  a  b
         * [0][1]  <- source pixels
         * [2][3]
         *
         * u_a is the x-coordinate of the first column.
         * u_b is the x-coordinate of the second column.
         *
         * [rgba]_a are the combined values of pixels 0 and 2
         *          wrt the current v-coordinate.
         * [rgba]_b are the combined values of pixels 1 and 3
         *          wrt the current v-coordinate.
         */
        gint    v = v_start,
                u_a, u_b;
        gdouble fv,
                r_a = 0., r_b = 0.,
                g_a = 0., g_b = 0.,
                b_a = 0., b_b = 0.,
                a_a = 0., a_b = 0.;

        fv = v_start - v;
        fv = MAX (fv, 0.);

        u_float   = u_start;
        u_a = u_b = (gint) u_float - 1;

        src_start = src_buf + v * src_rowstride;
        for (x = dest_w; x--; u_float += inverse [0][0])
          {
            gint    u;
            gdouble fu;

            u = u_float;
            if (u != u_a)
              {
                guint8 *src_ptr2;

                src_ptr  = src_start + (u << 2);
                src_ptr2 = src_ptr + src_rowstride;

                if (u == u_b)
                  {
                    u_a = u_b;
                    r_a = r_b;
                    g_a = g_b;
                    b_a = b_b;
                    a_a = a_b;
                  }
                else
                  {
                    u_a = u;
                    r_a = src_ptr [0] + (src_ptr2 [0] - src_ptr [0]) * fv;
                    g_a = src_ptr [1] + (src_ptr2 [1] - src_ptr [1]) * fv;
                    b_a = src_ptr [2] + (src_ptr2 [2] - src_ptr [2]) * fv;
                    a_a = src_ptr [3] + (src_ptr2 [3] - src_ptr [3]) * fv;
                  }

                u_b = u + 1;
                r_b = src_ptr [4] + (src_ptr2 [4] - src_ptr [4]) * fv;
                g_b = src_ptr [5] + (src_ptr2 [5] - src_ptr [5]) * fv;
                b_b = src_ptr [6] + (src_ptr2 [6] - src_ptr [6]) * fv;
                a_b = src_ptr [7] + (src_ptr2 [7] - src_ptr [7]) * fv;
              }

            fu = u_float - u;
            fu = MAX (fu, 0.);

            *dest_ptr++ = r_a + (r_b - r_a) * fu;
            *dest_ptr++ = g_a + (g_b - g_a) * fu;
            *dest_ptr++ = b_a + (b_b - b_a) * fu;
            *dest_ptr++ = a_a + (a_b - a_a) * fu;
          }
        dest_ptr += skip_left + skip_right;
      }
  else if (fabs (matrix [1][1]) > .5)
    for (x = dest_w; x--; u_start += inverse [0][0])
      {
        /* a [0][1]  <- source pixels
         * b [2][3]
         *
         * v_a is the y-coordinate of the first row.
         * v_b is the y-coordinate of the second row.
         *
         * [rgba]_a are the combined values of pixels 0 and 1
         *          wrt the current u-coordinate.
         * [rgba]_b are the combined values of pixels 2 and 3
         *          wrt the current u-coordinate.
         */
        gint    u = u_start,
                v_a, v_b;
        gdouble fu,
                r_a = 0., r_b = 0.,
                g_a = 0., g_b = 0.,
                b_a = 0., b_b = 0.,
                a_a = 0., a_b = 0.;

        fu = u_start - u;
        fu = MAX (fu, 0.);

        v_float   = v_start;
        v_a = v_b = (gint) v_float - 1;

        src_start = src_buf + (u << 2);
        for (y = dest_h; y--; v_float += inverse [1][1])
          {
            gint    v;
            gdouble fv;

            v = v_float;
            if (v != v_a)
              {
                src_ptr = src_start + v * src_rowstride;

                if (v == v_b)
                  {
                    v_a = v_b;
                    r_a = r_b;
                    g_a = g_b;
                    b_a = b_b;
                    a_a = a_b;
                  }
                else
                  {
                    v_a = v;
                    r_a = src_ptr [0] + (src_ptr [4] - src_ptr [0]) * fu;
                    g_a = src_ptr [1] + (src_ptr [5] - src_ptr [1]) * fu;
                    b_a = src_ptr [2] + (src_ptr [6] - src_ptr [2]) * fu;
                    a_a = src_ptr [3] + (src_ptr [7] - src_ptr [3]) * fu;
                  }

                src_ptr += src_rowstride;
                v_b = v + 1;
                r_b = src_ptr [0] + (src_ptr [4] - src_ptr [0]) * fu;
                g_b = src_ptr [1] + (src_ptr [5] - src_ptr [1]) * fu;
                b_b = src_ptr [2] + (src_ptr [6] - src_ptr [2]) * fu;
                a_b = src_ptr [3] + (src_ptr [7] - src_ptr [3]) * fu;
              }

            fv = v_float - v;
            fv = MAX (fv, 0.);

            dest_ptr [0] = r_a + (r_b - r_a) * fv;
            dest_ptr [1] = g_a + (g_b - g_a) * fv;
            dest_ptr [2] = b_a + (b_b - b_a) * fv;
            dest_ptr [3] = a_a + (a_b - a_a) * fv;
            dest_ptr += dest_rowstride;
          }
        dest_ptr = dest_start += 4;
      }
  else
    for (y = dest_h; y--; v_start += inverse [1][1])
      {
        gint    v = v_start;
        gdouble fv,
                fv_inv;

        fv = v_start - v;
        fv = MAX (fv, 0.);
        fv_inv = 1. - fv;

        u_float   = u_start;
        src_start = src_buf + v * src_rowstride;

        for (x = dest_w; x--; u_float += inverse [0][0])
          {
            gint    u = u_float;
            gdouble fu,
                    fu_inv,
                    factor0,
                    factor1,
                    factor2,
                    factor3,
                    r, g, b, a;

            fu = u_float - u;
            fu = MAX (fu, 0.);
            fu_inv = 1. - fu;

            factor0 = fv_inv * fu_inv;
            factor1 = fv_inv * fu;
            factor2 = fv * fu_inv;
            factor3 = fv * fu;

            src_ptr = src_start + (u << 2);
            r = src_ptr [0] * factor0;
            g = src_ptr [1] * factor0;
            b = src_ptr [2] * factor0;
            a = src_ptr [3] * factor0;

            r += src_ptr [4] * factor1;
            g += src_ptr [5] * factor1;
            b += src_ptr [6] * factor1;
            a += src_ptr [7] * factor1;

            src_ptr += src_rowstride;
            r += src_ptr [0] * factor2;
            g += src_ptr [1] * factor2;
            b += src_ptr [2] * factor2;
            a += src_ptr [3] * factor2;

            r += src_ptr [4] * factor3;
            g += src_ptr [5] * factor3;
            b += src_ptr [6] * factor3;
            a += src_ptr [7] * factor3;

            *dest_ptr++ = r;
            *dest_ptr++ = g;
            *dest_ptr++ = b;
            *dest_ptr++ = a;
          }
        dest_ptr += skip_left + skip_right;
      }

  gegl_buffer_set_fmt (dest, dest_buf, babl_format ("RGBA u8"));

  g_free (src_buf);
  g_free (dest_buf);
}
