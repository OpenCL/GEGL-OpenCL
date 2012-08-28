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
 * Copyright 1997 Miles O'Neal <meo@rru.com>  http://www.rru.com/~meo/
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

/*
 *  HURL Operation
 *      We just assign a random value at the current pixel
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_seed (seed, _("Seed"), _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),   0.0, 100.0, 3.0, _("Radomization"))

gegl_chant_int (repeat, _("Repeat"),   1, 100, 1, _("Repeat"))


#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "noise-hurl.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input" , babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
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

  gint i, cnt;

  GRand    *gr;

  gfloat   * GEGL_ALIGNED in_pixel;
  gfloat   * GEGL_ALIGNED out_pixel;

  gfloat red, green, blue, alpha;
  gfloat *out_pix;

  in_pixel      = in_buf;
  out_pixel     = out_buf;


  gr = g_rand_new_with_seed (o->seed);

  out_pix = out_pixel;

  for (i = 0; i < n_pixels*4; i++)
  {
    *out_pix = *in_pixel;
    out_pix += 1;
    in_pixel += 1;
  }


  for (cnt = 1; cnt <= o->repeat; cnt++)
  {
    out_pix = out_pixel;

    for (i = 0; i < n_pixels; i++)
    {
      red   = out_pix[0];
      green = out_pix[1];
      blue  = out_pix[2];
      alpha = out_pix[3];

      if (g_rand_double_range (gr, 0.0, 100.0) <= o->pct_random)
      {
        red   = g_rand_double_range (gr, 0.0, 1.0);
        green = g_rand_double_range (gr, 0.0, 1.0);
        blue  = g_rand_double_range (gr, 0.0, 1.0);
      }

      out_pix[0] = red;
      out_pix[1] = green;
      out_pix[2] = blue;
      out_pix[3] = alpha;

      out_pix += 4;
    }
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
      "name",       "gegl:noise-Hurl",
      "categories", "noise",
      "description", _("Completely randomize a fraction of pixels"),
      NULL);
}

#endif
