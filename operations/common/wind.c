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
 * Copyright Nigel Wetten
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (threshold, _("Threshold"), 0.0, 1.0, 0.2,
                   _("Higher values restrict the effect to fewer "
                     "areas of the image"))

gegl_chant_int (strength, _("Strength"), 1, 100, 4,
                _("Higher values increase the magnitude of the effect"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "wind.c"

#include "gegl-chant.h"
//#include <math.h>
#include <stdlib.h>


//assume leading
//assume has alpha
static void
get_derivative (gfloat   *pixel1,
                gfloat   *pixel2,
                gfloat   *derivative)
{
  gint i;
  for (i = 0; i < 4; i++)
    derivative[i] = pixel1[i] - pixel2[i];
}

static gboolean
threshold_exceeded (gfloat  *pixel1,
                    gfloat  *pixel2,
                    gfloat   threshold)
{
  gfloat derivative[4];
  gint i;
  gfloat sum;

  get_derivative (pixel1, pixel2, derivative);

  sum = 0;
  for (i = 0; i < 4; i++)
    sum += derivative[i];

  return ((sum / 4.0) > threshold);
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   = o->strength; //only from left for now

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

  gfloat *src_buf;
  gfloat *dst_buf;

  gint n_pixels = result->width * result->height;
  GeglRectangle src_rect;

  gfloat *blend_pix; //left edge
  gfloat *target_pix; //normally right edge
  gfloat *base_pix; //input pixel corresponding to write
  gfloat *dst_pix; //write target

  gint x, y;
  gint total_pixels;

  gfloat denominator;

  gint n;
  gint bleed_length_max;
  gint bleed_length;

  /* code to calculate each left edge and the associated bleed length
   that is randomly generated*/

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_pixels = src_rect.width * src_rect.height;

  src_buf = g_slice_alloc (4 * total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (4 * total_pixels * sizeof (gfloat));

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  //for each pixel
  base_pix = src_buf + 4 * o->strength;
  dst_pix = dst_buf;
  x = 0;
  y = 0;
  n_pixels = result->width * result->height;
  bleed_length = o->strength;
  while (n_pixels--)
    {
      /* need to check up to the max bleed length to see if there is a left edge
       which bleeds to the current pixel and bleed approriately */

      gint i;
      denominator = 1.0/(gfloat) bleed_length;;
      for (i = 0; i < 4; i++)
            dst_pix[i] = base_pix[i];
      for (n = 0; n < bleed_length; n++)
        {
          target_pix = base_pix - 4 * n;
          blend_pix = target_pix - 4 * 3;
          if (x < 3 + n)
            continue;

          if (threshold_exceeded (blend_pix, target_pix, o->threshold))
            {
              gfloat blend_color[4];
              gfloat blend_amount[4];
              for (i = 0; i < 4; i++)
                {
                  blend_color[i] = blend_pix[i];
                  blend_amount[i] = target_pix[i] - blend_pix[i];
                  blend_color[i] += blend_amount[i] * (gfloat) n * denominator;
                  dst_pix[i] = (2.0 * blend_color[i] + dst_pix[i])/3.0;
                }
            }
        }
      dst_pix += 4;
      /* update x and y coordinates */
      x++;
      base_pix += 4;
      if (x >= result->width)
        {
          x = 0;
          y++;
          base_pix += 4 * o->strength;
        }
    }
  gegl_buffer_set (output, result, 0,
                   babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (4 * total_pixels * sizeof (gfloat), src_buf);
  g_slice_free1 (4 * total_pixels * sizeof (gfloat), dst_buf);
  return TRUE;
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
                                 "categories" , "distort",
                                 "name"       , "gegl:wind",
                                 "description", _("Wind-like bleed effect"),
                                 NULL);
}
#endif
