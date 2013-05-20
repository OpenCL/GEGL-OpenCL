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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int    (holdness, _("Holdness"),
                   1, 8, 2,
                   _("Holdness"))

gegl_chant_double (lightness_distance, _("Lightness"),
                   0.0, 100.0, 40.0,
                   _("Lightness"))

gegl_chant_double (chroma_distance, _("Chroma"),
                   0.0, 100.0, 40.0,
                   _("Chroma"))

gegl_chant_double (hue_distance, _("Hue"),
                   0.0, 180.0,  3.0,
                   _("Hue"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE "noise-cielch.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static gdouble
randomize_value (gdouble     now,
                 gdouble     min,
                 gdouble     max,
                 gboolean    wraps_around,
                 gdouble     rand_max,
                 gint        holdness)
{
  gint    flag, i;
  gdouble rand_val, new_val, steps;

  steps = max - min + 0.5;
  rand_val = g_random_double ();

  for (i = 1; i < holdness; i++)
  {
    double tmp = g_random_double ();
    if (tmp < rand_val)
      rand_val = tmp;
  }

  flag = (g_random_double () < 0.5) ? -1 : 1;
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
                             babl_format ("CIE LCH(ab) alpha double"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("CIE LCH(ab) alpha double"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o  = GEGL_CHANT_PROPERTIES (operation);

  gint i;

  gdouble   * GEGL_ALIGNED in_pixel;
  gdouble   * GEGL_ALIGNED out_pixel;

  gdouble    lightness, chroma, hue, alpha;

  in_pixel      = in_buf;
  out_pixel     = out_buf;

  for (i = 0; i < n_pixels; i++)
  {
    lightness = in_pixel[0];
    chroma    = in_pixel[1];
    hue       = in_pixel[2];
    alpha     = in_pixel[3];

    if ((o->hue_distance > 0) && (chroma > 0))
      hue = randomize_value (hue, 0.0, 359.0, TRUE, o->hue_distance, o->holdness);

    if (o->chroma_distance > 0) {
      if (chroma == 0)
        hue = g_random_double_range (0.0, 360.0);
      chroma = randomize_value (chroma, 0.0, 100.0, FALSE, o->chroma_distance, o->holdness);
    }

    if (o->lightness_distance > 0)
      lightness = randomize_value (lightness, 0.0, 100.0, FALSE, o->lightness_distance, o->holdness);

    out_pixel[0] = lightness;
    out_pixel[1] = chroma;
    out_pixel[2] = hue;
    out_pixel[3] = alpha;

    in_pixel  += 4;
    out_pixel += 4;

  }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:noise-CIE_lch",
    "categories",  "noise",
    "description", _("Randomize lightness, chroma and hue independently"),
    NULL);
}

#endif
