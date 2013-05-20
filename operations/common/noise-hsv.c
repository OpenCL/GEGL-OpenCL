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
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int    (holdness, _("Holdness"),
                   1, 8, 2,
                   _("Holdness"))

gegl_chant_double (hue_distance, _("Hue"),
                   0.0, 180.0, 3.0,
                   _("Hue"))

gegl_chant_double (saturation_distance, _("Saturation"),
                   0.0, 1.0, 0.04,
                   _("Saturation"))

gegl_chant_double (value_distance, _("Value"),
                   0.0, 1.0, 0.04,
                   _("Value"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE "noise-hsv.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static gfloat
randomize_value (gfloat     now,
                 gfloat     min,
                 gfloat     max,
                 gboolean   wraps_around,
                 gfloat     rand_max,
                 gint       holdness)
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
  gegl_operation_set_format (operation, "input", babl_format ("HSVA float"));
  gegl_operation_set_format (operation, "output", babl_format ("HSVA float"));
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

  gfloat   * GEGL_ALIGNED in_pixel;
  gfloat   * GEGL_ALIGNED out_pixel;

  gfloat    hue, saturation, value, alpha;

  in_pixel      = in_buf;
  out_pixel     = out_buf;

  for (i = 0; i < n_pixels; i++)
  {
    hue        = in_pixel[0];
    saturation = in_pixel[1];
    value      = in_pixel[2];
    alpha      = in_pixel[3];

    /* there is no need for scattering hue of desaturated pixels here */
    if ((o->hue_distance > 0) && (saturation > 0))
      hue = randomize_value (hue, 0.0, 359.0, TRUE, o->hue_distance, o->holdness);

    /* desaturated pixels get random hue before increasing saturation */
    if (o->saturation_distance > 0) {
      if (saturation == 0)
        hue = g_random_double_range (0.0, 360.0);
      saturation = randomize_value (saturation, 0.0, 1.0, FALSE, o->saturation_distance, o->holdness);
    }

    if (o->value_distance > 0)
      value = randomize_value (value, 0.0, 1.0, FALSE, o->value_distance, o->holdness);

    out_pixel[0] = hue;
    out_pixel[1] = saturation;
    out_pixel[2] = value;
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

  operation_class->prepare = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-hsv",
    "categories", "noise",
    "description", _("Randomize hue, saturation and value independently"),
      NULL);
}

#endif
