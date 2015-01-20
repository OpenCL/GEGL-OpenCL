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
 *
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int   (holdness, _("Holdness"), 2)
   value_range (1, 8)

property_double (lightness_distance, _("Lightness"), 40.0)
   value_range  (0.0, 100.0)

property_double (chroma_distance, _("Chroma"), 40.0)
   value_range  (0.0, 100.0)

property_double (hue_distance, _("Hue"), 3.0)
   value_range  (0.0, 180.0)

property_seed   (seed, _("Random seed"), rand)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE noise-cie-lch.c

#include "gegl-op.h"
#include "gegl.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static gfloat
randomize_value (gfloat      now,
                 gfloat      min,
                 gfloat      max,
                 gboolean    wraps_around,
                 gfloat      rand_max,
                 gint        holdness,
                 gint        x,
                 gint        y,
                 gint        n,
                 GeglRandom *rand)
{
  gint    flag, i;
  gfloat rand_val, new_val, steps;

  steps = max - min + 0.5;
  rand_val = gegl_random_float (rand, x, y, 0, n++);

  for (i = 1; i < holdness; i++)
  {
    float tmp = gegl_random_float (rand, x, y, 0, n++);
    if (tmp < rand_val)
      rand_val = tmp;
  }

  flag = (gegl_random_float (rand, x, y, 0, n) < 0.5) ? -1 : 1;
  new_val = now + flag * fmod (rand_max * rand_val, steps);

  if (new_val < min)
  {
    if (wraps_around)
      new_val += steps;
    else
      new_val = min;
  }

  if (max < new_val)
  {
    if (wraps_around)
      new_val -= steps;
    else
      new_val = max;
  }

  return new_val;
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input" ,
                             babl_format ("CIE LCH(ab) alpha float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("CIE LCH(ab) alpha float"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat         *GEGL_ALIGNED in_pixel;
  gfloat         *GEGL_ALIGNED out_pixel;
  GeglRectangle   whole_region;
  gfloat          lightness, chroma, hue, alpha;
  gint            i;
  gint            x, y;

  in_pixel  = in_buf;
  out_pixel = out_buf;

  x = roi->x;
  y = roi->y;

  whole_region = *(gegl_operation_source_get_bounding_box (operation, "input"));

  for (i = 0; i < n_pixels; i++)
  {
    /* n is independent from the roi, but from the whole image */
    gint n = (3 * o->holdness + 4) * (x + whole_region.width * y);

    lightness = in_pixel[0];
    chroma    = in_pixel[1];
    hue       = in_pixel[2];
    alpha     = in_pixel[3];

    if ((o->hue_distance > 0) && (chroma > 0))
      hue = randomize_value (hue, 0.0, 359.0, TRUE, o->hue_distance,
                             o->holdness, x, y, n, o->rand);

    n += o->holdness + 1;
    if (o->chroma_distance > 0) {
      if (chroma == 0)
        hue = gegl_random_float_range (o->rand, x, y, 0, n, 0.0, 360.0);
      chroma = randomize_value (chroma, 0.0, 100.0, FALSE, o->chroma_distance,
                                o->holdness, x, y, n+1, o->rand);
    }

    n += o->holdness + 2;
    if (o->lightness_distance > 0)
      lightness = randomize_value (lightness, 0.0, 100.0, FALSE,
                                   o->lightness_distance, o->holdness,
                                   x, y, n, o->rand);

    /*n += o->holdness + 1*/
    out_pixel[0] = lightness;
    out_pixel[1] = chroma;
    out_pixel[2] = hue;
    out_pixel[3] = alpha;

    in_pixel  += 4;
    out_pixel += 4;

    x++;
    if (x >= roi->x + roi->width)
      {
        x = roi->x;
        y++;
      }
  }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:noise-cie-lch",
    "title",       _("Add CIE Lch Noise"),
    "categories",  "noise",
    "description", _("Randomize lightness, chroma and hue independently"),
    NULL);
}

#endif
