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
 * Copyright 2012,2016 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (correlated, _("Correlated noise"), FALSE)

property_boolean (independent, _("Independent RGB"), TRUE)
   description (_("Control amount of noise for each RGB channel separately"))

property_boolean (linear, _("Linear RGB"), TRUE)
   description (_("Operate on linearized RGB color data"))

property_boolean (gaussian, _("Gaussian distribution"), TRUE)
   description (_("Use a gaussian noise distribution, when unticked a linear noise distribution is used instead"))

property_double (red, _("Red"), 0.20)
   value_range  (0.0, 1.0)

property_double (green, _("Green"), 0.20)
   value_range  (0.0, 1.0)

property_double (blue, _("Blue"), 0.20)
   value_range  (0.0, 1.0)

property_double (alpha, _("Alpha"), 0.0)
   value_range  (0.0, 1.0)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     noise_rgb
#define GEGL_OP_C_SOURCE noise-rgb.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*
 * Return a Gaussian (aka normal) distributed random variable.
 *
 * Adapted from gauss.c included in GNU scientific library.
 *
 * Ratio method (Kinderman-Monahan); see Knuth v2, 3rd ed, p130
 * K+M, ACM Trans Math Software 3 (1977) 257-260.
*/
static gfloat
noise_gauss (GeglRandom *rand, int *i, int xx, int yy)
{
  gfloat u, v, x;

  do
  {
    v = gegl_random_float (rand, xx, yy, 0, (*i)++);

    do
      u = gegl_random_float (rand, xx, yy, 0, (*i)++);
    while (u == 0);

    /* Const 1.715... = sqrt(8/e) */
    x = 1.71552776992141359295 * (v - 0.5f) / u;
  }
  while (x * x > -4.0f * logf (u));

  return x;
}

static gfloat
noise_linear (GeglRandom *rand, int *i, int xx, int yy)
{
  return gegl_random_float (rand, xx, yy, 0, (*i)++) * 2 - 1.0;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  if (o->linear)
    {
      gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
      gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
    }
  else
    {
      gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
      gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
    }
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

  gdouble  noise_coeff = 0.0;
  int      rint = 0;
  gint     b, i;
  gint     x, y;
  gdouble  noise[4];
  gfloat   tmp;
  gfloat   * GEGL_ALIGNED in_pixel;
  gfloat   * GEGL_ALIGNED out_pixel;
  gfloat   (*noise_fun) (GeglRandom *rand, int *i, int xx, int yy) = noise_gauss;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  noise[0] = o->red;
  noise[1] = o->green;
  noise[2] = o->blue;
  noise[3] = o->alpha;

  if (o->gaussian == FALSE)
    noise_fun = noise_linear;

  x = roi->x;
  y = roi->y;

  for (i=0; i<n_pixels; i++)
  {
    for (b = 0; b < 4; b++)
    {
      if (b == 0 || o->independent || b == 3 )
         noise_coeff = noise[b] * noise_fun (o->rand, &rint, x, y) * 0.5;

      if (noise[b] > 0.0)
      {
        if (o->correlated)
        {
          tmp = (in_pixel[b] + (in_pixel[b] * (noise_coeff / 0.5)) );
        }
        else
        {
          tmp = (in_pixel[b] + noise_coeff );
        }

        out_pixel[b] = CLAMP(tmp, 0.0, 1.0);
      }
      else
      {
        out_pixel[b] = in_pixel[b];
      }
    }

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
    "name",        "gegl:noise-rgb",
    "title",       _("Add RGB Noise"),
    "categories",  "noise",
    "description", _("Distort colors by random amounts"),
    NULL);
}

#endif
