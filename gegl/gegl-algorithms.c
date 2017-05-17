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
 * Copyright 2006,2007,2015 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include <babl/babl.h>

#include "gegl-types.h"
#include "gegl-types-internal.h"
#include "gegl-algorithms.h"

#include <math.h>

void gegl_downscale_2x2 (const Babl *format,
                         gint    src_width,
                         gint    src_height,
                         guchar *src_data,
                         gint    src_rowstride,
                         guchar *dst_data,
                         gint    dst_rowstride)
{
  const gint  bpp = babl_format_get_bytes_per_pixel (format);
  const Babl *comp_type = babl_format_get_type (format, 0);

  if (comp_type == gegl_babl_float())
    gegl_downscale_2x2_float (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
  else if (comp_type == gegl_babl_u8())
    gegl_downscale_2x2_u8 (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
  else if (comp_type == gegl_babl_u16())
    gegl_downscale_2x2_u16 (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
  else if (comp_type == gegl_babl_u32())
    gegl_downscale_2x2_u32 (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
  else if (comp_type == gegl_babl_double())
    gegl_downscale_2x2_double (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
  else
    gegl_downscale_2x2_nearest (bpp, src_width, src_height, src_data, src_rowstride, dst_data, dst_rowstride);
}

void
gegl_downscale_2x2_nearest (gint    bpp,
                            gint    src_width,
                            gint    src_height,
                            guchar *src_data,
                            gint    src_rowstride,
                            guchar *dst_data,
                            gint    dst_rowstride)
{
  gint y;

  for (y = 0; y < src_height / 2; y++)
    {
      gint x;
      guchar *src = src_data;
      guchar *dst = dst_data;

      for (x = 0; x < src_width / 2; x++)
        {
          memcpy (dst, src, bpp);
          dst += bpp;
          src += bpp * 2;
        }

      dst_data += dst_rowstride;
      src_data += src_rowstride * 2;
    }
}

void gegl_resample_boxfilter (guchar              *dest_buf,
                              const guchar        *source_buf,
                              const GeglRectangle *dst_rect,
                              const GeglRectangle *src_rect,
                              gint                 s_rowstride,
                              gdouble              scale,
                              const Babl          *format,
                              gint                 d_rowstride)
{
  const Babl *comp_type  = babl_format_get_type (format, 0);
  const gint bpp = babl_format_get_bytes_per_pixel (format);

  if (comp_type == gegl_babl_float())
    gegl_resample_boxfilter_float (dest_buf, source_buf, dst_rect, src_rect,
                                   s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u8())
    gegl_resample_boxfilter_u8 (dest_buf, source_buf, dst_rect, src_rect,
                                s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u16())
    gegl_resample_boxfilter_u16 (dest_buf, source_buf, dst_rect, src_rect,
                                 s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u32())
    gegl_resample_boxfilter_u32 (dest_buf, source_buf, dst_rect, src_rect,
                                 s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_double())
    gegl_resample_boxfilter_double (dest_buf, source_buf, dst_rect, src_rect,
                                    s_rowstride, scale, bpp, d_rowstride);
  else
    gegl_resample_nearest (dest_buf, source_buf, dst_rect, src_rect,
                           s_rowstride, scale, bpp, d_rowstride);
}

void gegl_resample_bilinear (guchar              *dest_buf,
                             const guchar        *source_buf,
                             const GeglRectangle *dst_rect,
                             const GeglRectangle *src_rect,
                             gint                 s_rowstride,
                             gdouble              scale,
                             const Babl          *format,
                             gint                 d_rowstride)
{
  const Babl *comp_type  = babl_format_get_type (format, 0);
  const gint bpp = babl_format_get_bytes_per_pixel (format);

  if (comp_type == gegl_babl_float ())
    gegl_resample_bilinear_float (dest_buf, source_buf, dst_rect, src_rect,
                                  s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u8 ())
    gegl_resample_bilinear_u8 (dest_buf, source_buf, dst_rect, src_rect,
                               s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u16 ())
    gegl_resample_bilinear_u16 (dest_buf, source_buf, dst_rect, src_rect,
                                s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_u32 ())
    gegl_resample_bilinear_u32 (dest_buf, source_buf, dst_rect, src_rect,
                                s_rowstride, scale, bpp, d_rowstride);
  else if (comp_type == gegl_babl_double ())
    gegl_resample_bilinear_double (dest_buf, source_buf, dst_rect, src_rect,
                                   s_rowstride, scale, bpp, d_rowstride);
  else
    gegl_resample_nearest (dest_buf, source_buf, dst_rect, src_rect,
                           s_rowstride, scale, bpp, d_rowstride);
}

static inline int int_floorf (float x)
{
  int i = (int)x; /* truncate */
  return i - ( i > x ); /* convert trunc to floor */
}

void
gegl_resample_nearest (guchar              *dst,
                       const guchar        *src,
                       const GeglRectangle *dst_rect,
                       const GeglRectangle *src_rect,
                       const gint           src_stride,
                       const gdouble        scale,
                       const gint           bpp,
                       const gint           dst_stride)
{
  int i, j;

  for (i = 0; i < dst_rect->height; i++)
    {
      const gfloat sy = (dst_rect->y + .5 + i) / scale - src_rect->y;
      const gint   ii = int_floorf (sy + GEGL_SCALE_EPSILON);

      for (j = 0; j < dst_rect->width; j++)
        {
          const gfloat sx = (dst_rect->x + .5 + j) / scale - src_rect->x;
          const gint   jj = int_floorf (sx + GEGL_SCALE_EPSILON);

          memcpy (&dst[i * dst_stride + j * bpp],
                  &src[ii * src_stride + jj * bpp],
                  bpp);
        }
    }
}

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_double
#define BOXFILTER_TYPE       gdouble
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_float
#define BOXFILTER_TYPE       gfloat
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u8
#define BOXFILTER_TYPE       guchar
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u16
#define BOXFILTER_TYPE       guint16
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u32
#define BOXFILTER_TYPE       guint32
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_double
#define DOWNSCALE_TYPE     gdouble
#define DOWNSCALE_SUM      gdouble
#define DOWNSCALE_DIVISOR  4.0
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_float
#define DOWNSCALE_TYPE     gfloat
#define DOWNSCALE_SUM      gfloat
#define DOWNSCALE_DIVISOR  4.0f
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u32
#define DOWNSCALE_TYPE     guint32
#define DOWNSCALE_SUM      guint64
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u16
#define DOWNSCALE_TYPE     guint16
#define DOWNSCALE_SUM      guint
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u8
#define DOWNSCALE_TYPE     guint8
#define DOWNSCALE_SUM      guint
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR


#define BILINEAR_FUNCNAME   gegl_resample_bilinear_double
#define BILINEAR_TYPE       gdouble
#define BILINEAR_ROUND(val) (val)
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_float
#define BILINEAR_TYPE       gfloat
#define BILINEAR_ROUND(val) (val)
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u8
#define BILINEAR_TYPE       guchar
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u16
#define BILINEAR_TYPE       guint16
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u32
#define BILINEAR_TYPE       guint32
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

