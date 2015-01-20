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

#ifdef GEGL_PROPERTIES

property_int  (holdness, _("Holdness"), 2)
  value_range (1, 8)

property_double (hue_distance, _("Hue"), 3.0)
  value_range   (0.0, 180.0)

property_double (saturation_distance, _("Saturation"), 0.04)
  value_range   (0.0, 1.0)

property_double (value_distance, _("Value"), 0.04)
  value_range   (0.0, 1.0)

property_seed   (seed, _("Random seed"), rand)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE noise-hsv.c

#include "gegl-op.h"
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

  steps = max - min;
  rand_val = gegl_random_float (rand, x, y, 0, n++);

  for (i = 1; i < holdness; i++)
  {
    gfloat tmp = gegl_random_float (rand, x, y, 0, n++);
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
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  GeglRectangle whole_region;
  gint i;
  gint x, y;

  gfloat   * GEGL_ALIGNED in_pixel;
  gfloat   * GEGL_ALIGNED out_pixel;

  gfloat    hue, saturation, value, alpha;

  in_pixel      = in_buf;
  out_pixel     = out_buf;

  x = roi->x;
  y = roi->y;

  whole_region = *(gegl_operation_source_get_bounding_box (operation, "input"));

  for (i = 0; i < n_pixels; i++)
  {
    /* n is independent from the roi, but from the whole image */
    gint n = (3 * o->holdness + 4) * (x + whole_region.width * y);

    hue        = in_pixel[0];
    saturation = in_pixel[1];
    value      = in_pixel[2];
    alpha      = in_pixel[3];

    /* there is no need for scattering hue of desaturated pixels here */
    if ((o->hue_distance > 0) && (saturation > 0))
      hue = randomize_value (hue, 0.0, 1.0, TRUE, o->hue_distance / 360.0,
                             o->holdness, x, y, n, o->rand);

    n += o->holdness + 1;
    /* desaturated pixels get random hue before increasing saturation */
    if (o->saturation_distance > 0) {
      if (saturation == 0)
        hue = gegl_random_float_range (o->rand, x, y, 0, n, 0.0, 1.0);
      saturation = randomize_value (saturation, 0.0, 1.0, FALSE,
                                    o->saturation_distance, o->holdness,
                                    x, y, n+1, o->rand);
    }

    n += o->holdness + 2;
    if (o->value_distance > 0)
      value = randomize_value (value, 0.0, 1.0, FALSE, o->value_distance,
                               o->holdness, x, y, n, o->rand);

    out_pixel[0] = hue;
    out_pixel[1] = saturation;
    out_pixel[2] = value;
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

  operation_class->prepare = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-hsv",
    "title",      _("Add HSV Noise"),
    "categories", "noise",
    "description", _("Randomize hue, saturation and value independently"),
      NULL);
}

#endif
