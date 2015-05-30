/* This file is an image processing operation for GEGL
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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_int  (radius, _("Radius"), 3)
  value_range (1, 100)
  description (_("Radius of square pixel region (width and height will be radius*2+1)"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE     median-blur.c

#include "gegl-op.h"

#define N_BINS  1024

typedef struct
{
  gint       elems[3][N_BINS]; 
  gint       count;
  gint       xmin;
  gint       ymin;
  gint       xmax;
  gint       ymax;
} Histogram; 

typedef enum
{
  LEFT_TO_RIGHT,
  RIGHT_TO_LEFT,
  TOP_TO_BOTTOM
} Direction;

static inline gfloat
histogram_get_median (Histogram *hist, gint component)
{
  gint count = hist->count;
  gint i;
  gint sum = 0;

  count = (count + 1) / 2;

  i = 0;
  while ((sum += hist->elems[component][i]) < count)
    i++;

  return (gfloat) i / (gfloat) N_BINS;
}

static inline void
histogram_add_val (Histogram     *hist,
                   const gfloat  *src,
                   GeglRectangle *rect,
                   gint           bpp,
                   gint           x,
                   gint           y)
{
  const gint pos = (x + y * rect->width) * bpp;
  gint c;

  for (c = 0; c < 3; c++)
    {
      gfloat value = *(src + pos + c);
      gint idx     = (gint) (CLAMP (value, 0.0, 1.0) * (N_BINS - 1));
      hist->elems[c][idx]++;
    }
  hist->count++;
}

static inline void
histogram_del_val (Histogram     *hist,
                   const gfloat  *src,
                   GeglRectangle *rect,
                   gint           bpp,
                   gint           x,
                   gint           y)
{
  const gint pos = (x + y * rect->width) * bpp;
  gint c;

  for (c = 0; c < 3; c++)
    {
      gfloat value = *(src + pos + c);
      gint idx     = (gint) (CLAMP (value, 0.0, 1.0) * (N_BINS - 1));
      hist->elems[c][idx]--;
    }
  hist->count--;
}

static inline void
histogram_add_vals (Histogram     *hist,
                    const gfloat  *src,
                    GeglRectangle *rect,
                    gint           bpp,
                    gint           xmin,
                    gint           ymin,
                    gint           xmax,
                    gint           ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          histogram_add_val (hist, src, rect, bpp, x, y);
        }
    }
}

static inline void
histogram_del_vals (Histogram     *hist,
                    const gfloat  *src,
                    GeglRectangle *rect,
                    gint           bpp,
                    gint           xmin,
                    gint           ymin,
                    gint           xmax,
                    gint           ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          histogram_del_val (hist, src, rect, bpp, x, y);
        }
    }
}

static inline void
histogram_update (Histogram     *hist,
                  const gfloat  *src,
                  GeglRectangle *rect,
                  gint           bpp,
                  gint           xmin,
                  gint           ymin,
                  gint           xmax,
                  gint           ymax,
                  Direction      dir)
{
  switch (dir)
    {
      case LEFT_TO_RIGHT:
        histogram_del_vals (hist, src, rect, bpp,
                            hist->xmin, hist->ymin,
                            hist->xmin, hist->ymax);

        histogram_add_vals (hist, src, rect, bpp,
                            xmax, ymin,
                            xmax, ymax);
        break;

      case RIGHT_TO_LEFT:
        histogram_del_vals (hist, src, rect, bpp,
                            hist->xmax, hist->ymin,
                            hist->xmax, hist->ymax);

        histogram_add_vals (hist, src, rect, bpp,
                            xmin, ymin,
                            xmin, ymax);
        break;

      case TOP_TO_BOTTOM:
        histogram_del_vals (hist, src, rect, bpp,
                            hist->xmin, hist->ymin,
                            hist->xmax, hist->ymin);

        histogram_add_vals (hist, src, rect, bpp,
                            xmin, ymax,
                            xmax, ymax);
        break;
    }

  hist->xmin = xmin;
  hist->ymin = ymin;
  hist->xmax = xmax;
  hist->ymax = ymax;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties     *o         = GEGL_PROPERTIES (operation);
  const Babl         *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl         *format    = babl_format ("RGB float");;

  area->left   =
  area->right  =
  area->top    =
  area->bottom = o->radius;

  if (in_format)
    {
      if (babl_format_has_alpha (in_format))
        format = babl_format ("RGBA float");
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
  {
    result = *in_rect;
  }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "input");
  gint n_components  = babl_format_get_n_components (format);
  gboolean has_alpha = babl_format_has_alpha (format);

  gfloat *src_buf;
  gfloat *dst_buf;
  gint    n_pixels;
  GeglRectangle src_rect; 

  Histogram *hist;
  Direction  dir;
  
  gint src_x, src_y;
  gint dst_x, dst_y;
  gint dst_idx, src_idx;
  gint xmin, ymin, xmax, ymax;

  src_rect = gegl_operation_get_required_for_output (operation, "input", roi);
  n_pixels = roi->width * roi->height;
  dst_buf = g_new0 (gfloat, n_pixels * n_components);
  src_buf = g_new0 (gfloat, src_rect.width * src_rect.height * n_components);

  gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  hist = g_slice_new0 (Histogram);

  dst_x = 0;
  dst_y = 0;
  src_x = o->radius;
  src_y = o->radius;

  /* compute the first window */

  hist->xmin = src_x - o->radius;
  hist->ymin = src_y - o->radius;
  hist->xmax = src_x + o->radius;
  hist->ymax = src_y + o->radius;

  histogram_add_vals (hist, src_buf, &src_rect, n_components,
                      hist->xmin, hist->ymin,
                      hist->xmax, hist->ymax);

  dst_idx = (dst_x + dst_y * roi->width) * n_components;

  dst_buf[dst_idx]     = histogram_get_median (hist, 0);
  dst_buf[dst_idx + 1] = histogram_get_median (hist, 1);
  dst_buf[dst_idx + 2] = histogram_get_median (hist, 2);
 
  if (has_alpha)
    {
      src_idx = (src_x + src_y * src_rect.width) * n_components;
      dst_buf[dst_idx + 3] = src_buf[src_idx + 3];
    }

  n_pixels--;
  dir = LEFT_TO_RIGHT;

  while (n_pixels--)
    {
      /* move the src coords based on current direction and positions */
      if (dir == LEFT_TO_RIGHT)
        {
          if (dst_x != roi->width - 1)
            {
              src_x++;
              dst_x++;
            }
          else
            {
              src_y++;
              dst_y++;
              dir = TOP_TO_BOTTOM; 
            }
        }
      else if (dir == TOP_TO_BOTTOM)
        {
          if (dst_x == 0)
            {
              src_x++;
              dst_x++;
              dir = LEFT_TO_RIGHT;
            }
          else
            {
              src_x--;
              dst_x--;
              dir = RIGHT_TO_LEFT;
            }
        }
      else if (dir == RIGHT_TO_LEFT)
        {
          if (dst_x != 0)
            {
              src_x--;
              dst_x--;
            }
          else
            {
              src_y++;
              dst_y++;
              dir = TOP_TO_BOTTOM; 
            }
        }

      xmin = src_x - o->radius;
      ymin = src_y - o->radius;
      xmax = src_x + o->radius;
      ymax = src_y + o->radius;

      histogram_update (hist, src_buf, &src_rect, n_components,
                        xmin, ymin, xmax, ymax, dir);

      dst_idx = (dst_x + dst_y * roi->width) * n_components;

      dst_buf[dst_idx]     = histogram_get_median (hist, 0);
      dst_buf[dst_idx + 1] = histogram_get_median (hist, 1);
      dst_buf[dst_idx + 2] = histogram_get_median (hist, 2);
     
      if (has_alpha)
        {
          src_idx = (src_x + src_y * src_rect.width) * n_components;
          dst_buf[dst_idx + 3] = src_buf[src_idx + 3];
        }
    }

  gegl_buffer_set (output, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
  
  g_free (src_buf);
  g_free (dst_buf);
  g_slice_free (Histogram, hist);
 
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:median-blur",
    "title",       _("Median Blur"),
    "categories",  "blur",
    "description", _("Blur resulting from computing the median "
                     "color of in a square neighbourhood."),
    NULL);
}

#endif
