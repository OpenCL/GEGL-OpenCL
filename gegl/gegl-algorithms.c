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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include <babl/babl.h>

#include "gegl-types.h"
#include "gegl-algorithms.h"

#include <math.h>

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
  guint num_componenets;

  void (*resample_boxfilter_func) (guchar              *dest_buf,
                                   const guchar        *source_buf,
                                   const GeglRectangle *dst_rect,
                                   const GeglRectangle *src_rect,
                                   gint                 s_rowstride,
                                   gdouble              scale,
                                   gint                 components,
                                   gint                 d_rowstride) = NULL;

  if (comp_type == babl_type ("u8"))
    {
      resample_boxfilter_func = gegl_resample_boxfilter_u8;
    }
  else if (comp_type == babl_type ("u16"))
    {
      resample_boxfilter_func = gegl_resample_boxfilter_u16;
    }
  else if (comp_type == babl_type ("u32"))
    {
      resample_boxfilter_func = gegl_resample_boxfilter_u32;
    }
  else if (comp_type == babl_type ("float"))
    {
      resample_boxfilter_func = gegl_resample_boxfilter_float;
    }
  else
    {
      gegl_resample_nearest (dest_buf,
                             source_buf,
                             dst_rect,
                             src_rect,
                             s_rowstride,
                             scale,
                             babl_format_get_bytes_per_pixel (format),
                             d_rowstride);
      return;
    }

  num_componenets = babl_format_get_n_components (format);

  resample_boxfilter_func (dest_buf,
                           source_buf,
                           dst_rect,
                           src_rect,
                           s_rowstride,
                           scale,
                           num_componenets,
                           d_rowstride);
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
      const gdouble sy = (dst_rect->y + .5 + i) / scale - src_rect->y;
      const gint    ii = floor (sy + GEGL_SCALE_EPSILON);

      for (j = 0; j < dst_rect->width; j++)
        {
          const gdouble sx = (dst_rect->x + .5 + j) / scale - src_rect->x;
          const gint    jj = floor (sx + GEGL_SCALE_EPSILON);

          memcpy (&dst[i * dst_stride + j * bpp],
                  &src[ii * src_stride + jj * bpp],
                  bpp);
        }
    }
}

/*
#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_double
#define BOXFILTER_TYPE       gdouble
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND
*/

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_float
#define BOXFILTER_TYPE       gfloat
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u8
#define BOXFILTER_TYPE       guchar
#define BOXFILTER_ROUND(val) (lrint(val))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u16
#define BOXFILTER_TYPE       guint16
#define BOXFILTER_ROUND(val) (lrint(val))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u32
#define BOXFILTER_TYPE       guint32
#define BOXFILTER_ROUND(val) (lrint(val))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND