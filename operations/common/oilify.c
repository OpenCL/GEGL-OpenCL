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
 * Copyright 1995 Spencer Kimball and Peter Mattis
 * Copyright 1996 Torsten Martinsen
 * Copyright 2007 Daniel Richard G.
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (mask_radius, _("Mask Radius"), 1, 25, 4,
                _("Radius of circle around pixel"))

gegl_chant_double (exponent, _("Exponent"), 1.0, 20.0, 8.0,
                   _("Exponent"))

gegl_chant_boolean (use_inten, _("Intensity Mode"), TRUE,
                    _("Use pixel luminance values"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "oilify.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>

#define NUM_INTENSITIES       256

/* Get the pixel from x, y offset from the center pixel src_pix */
static void
get_pixel(gint x,
          gint y,
          gint buf_width,
          gfloat* src_begin,
          gfloat* dst)
{
  gint b;
  gfloat* src = src_begin + 4*(x + buf_width*y);
  for (b = 0; b < 4; b++)
    {
      dst[b] = src[b];
    }
}

static void
get_pixel_inten(gint x,
                gint y,
                gint buf_width,
                gfloat* inten_begin,
                gfloat* dst)
{
  *dst = *(inten_begin + (x + buf_width*y));
}

static void
oilify_pixel (gint x,
              gint y,
              GeglRectangle *whole_rect,
              gboolean use_inten,
              gdouble radius,
              gdouble exponent,
              gint buf_width,
              gfloat *src_buf,
              gfloat *inten_buf,
              gfloat *dst_pixel)
{
  gint hist[4][NUM_INTENSITIES];
  gfloat cumulative_rgb[4][NUM_INTENSITIES];
  gint hist_inten[NUM_INTENSITIES];
  gfloat mult_inten[NUM_INTENSITIES];
  gfloat temp_pixel[4];
  gfloat temp_inten_pixel;
  gint ceil_radius = ceil(radius);
  gdouble radius_sq = radius*radius;
  gint i, j, b;
  gint hist_max[4];
  gint inten_max;
  gint intensity;
  gfloat sum;
  gfloat ratio;
  gfloat weight;
  gfloat color;
  gfloat result;
  gfloat div;
  for (i = 0; i < NUM_INTENSITIES; i++)
    {
      hist_inten[i] = 0;
      for (b = 0; b < 4; b++)
        {
          hist[b][i] = 0;
          cumulative_rgb[b][i] = 0.0;
        }
    }

  /* calculate histograms */
  for (i = -ceil_radius; i <= ceil_radius; i++)
    {
      for (j = -ceil_radius; j <= ceil_radius; j++)
        {
          if (i*i + j*j <= radius_sq)
            {
	      get_pixel (x + i,
                         y + j,
                         buf_width,
                         src_buf,
                         temp_pixel);

              if (use_inten)
                {
                  get_pixel_inten (x + i,
                                   y + j,
                                   buf_width,
                                   inten_buf,
                                   &temp_inten_pixel);
                  intensity = temp_inten_pixel * NUM_INTENSITIES;
                  hist_inten[intensity]++;
                  for (b = 0; b < 4; b++)
                    {
                      cumulative_rgb[b][intensity] += temp_pixel[b];
                    }
                }
              else
                {
                  for (b = 0; b < 4; b++)
                    {
                      intensity = temp_pixel[b] * NUM_INTENSITIES;
                      hist[b][intensity]++;
                    }
                }
            }
        }
    }

  inten_max = 1;

  /* calculated maximums */
  for (i = 0; i < NUM_INTENSITIES; i++) {
    inten_max = MAX (inten_max, hist_inten[i]);
  }

  for (b = 0; b < 4; b++)
    {
      hist_max[b] = 1;
      for (i = 0; i < NUM_INTENSITIES; i++) {
        hist_max[b] = MAX (hist_max[b], hist[b][i]);
      }
    }

  /* calculate weight and use it to set the pixel */
  div = 0.0;
  for (i = 0; i < NUM_INTENSITIES; i++)
    {
      if (use_inten && hist_inten[i] > 0)
        {
          ratio = (gfloat) hist_inten[i] / (gfloat) inten_max;
          weight = pow (ratio, exponent);
          mult_inten[i] = weight / (gfloat) hist_inten[i];
          div += weight;
        }
    }
  for (b = 0; b < 4; b++)
    {
      sum = 0.0;
      color = 0.0;
      if (use_inten)
        {
          for (i = 0; i < NUM_INTENSITIES; i++)
            {
              if (hist_inten[i] > 0)
                color += mult_inten[i] * cumulative_rgb[b][i];
            }
          dst_pixel[b] = color/div;
        }
      else
        {
          div = 0.0;
          for (i = 0; i < NUM_INTENSITIES; i++)
            {
              ratio = (gfloat) hist[b][i] / (gfloat) hist_max[b];
              weight = pow (ratio, exponent);
              sum += weight * (gfloat) i;
              div += weight;
            }
          result = sum / (gfloat) NUM_INTENSITIES;
          dst_pixel[b] = result/div;
        }
    }
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
    op_area->right  =
    op_area->top    =
    op_area->bottom = o->mask_radius;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglRectangle* whole_rect;
  gint x = o->mask_radius; /* initial x                   */
  gint y = o->mask_radius; /*           and y coordinates */
  gfloat *src_buf;
  gfloat *dst_buf;
  gfloat *inten_buf;
  gfloat *out_pixel;
  gint n_pixels = result->width * result->height;
  GeglRectangle src_rect;
  gint total_pixels;

  whole_rect = gegl_operation_source_get_bounding_box (operation, "input");

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_pixels = src_rect.width * src_rect.height;

  src_buf = g_slice_alloc (4 * total_pixels * sizeof(gfloat));
  dst_buf = g_slice_alloc (4 * total_pixels * sizeof(gfloat));
  inten_buf = g_slice_alloc (total_pixels * sizeof(gfloat));

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("Y float"),
                   inten_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  out_pixel = dst_buf;
  while (n_pixels--)
    {
      oilify_pixel(x, y, whole_rect, o->use_inten, o->mask_radius, o->exponent,
                   src_rect.width, src_buf, inten_buf, out_pixel);

      out_pixel += 4;

      /* update x and y coordinates */
      x++;
      if (x >= result->width + o->mask_radius)
        {
          x=o->mask_radius;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0,
                   babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (4 * total_pixels * sizeof(gfloat), src_buf);
  g_slice_free1 (4 * total_pixels * sizeof(gfloat), dst_buf);
  g_slice_free1 (total_pixels * sizeof(gfloat), inten_buf);

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
                                 "categories" , "artistic",
                                 "name"       , "gegl:oilify",
                                 "description", _("Emulate an oil painting"),
                                 NULL);
}
#endif
