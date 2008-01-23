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
 * Copyright 2007 Mark Probst <mark.probst@gmail.com>
 */
#if GEGL_CHANT_PROPERTIES

  gegl_chant_int (sampling_points, 0, 65536, 0, "Number of curve sampling points.  0 for exact calculation.")
  gegl_chant_curve (curve, "The contrast curve.")

#else

#define GEGL_CHANT_NAME          contrast_curve
#define GEGL_CHANT_SELF          "contrast-curve.c"
#define GEGL_CHANT_DESCRIPTION   "Adjusts the contrast of the image according to a curve."
#define GEGL_CHANT_CATEGORIES    "color"

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_PREPARE

#include "gegl-old-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("YA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation *op,
         void          *in_buf,
         void          *out_buf,
         glong          samples)
{
  GeglChantOperation *self;
  gint i;
  gfloat *in  = in_buf;
  gfloat *out = out_buf;
  gint num_sampling_points;
  gdouble *xs, *ys;

  self = GEGL_CHANT_OPERATION (op);

  num_sampling_points = self->sampling_points;

  if (num_sampling_points > 0)
  {
    xs = g_new(gdouble, num_sampling_points);
    ys = g_new(gdouble, num_sampling_points);

    gegl_curve_calc_values(self->curve, 0.0, 1.0, num_sampling_points, xs, ys);

    g_free(xs);

    for (i=0; i<samples; i++)
    {
      gint x = in[0] * num_sampling_points;
      gfloat y;

      if (x < 0)
       y = ys[0];
      else if (x >= num_sampling_points)
       y = ys[num_sampling_points - 1];
      else
       y = ys[x];

      out[0] = y;
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

    g_free(ys);
  }
  else
    for (i=0; i<samples; i++)
    {
      gfloat u = in[0];

      out[0] = gegl_curve_calc_value(self->curve, u);
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

  return TRUE;
}

#endif
