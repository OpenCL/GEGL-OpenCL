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

gegl_chant_seed   (seed, _("Seed"), _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),
                   0.0, 100.0, 50.0, _("Randomization"))

gegl_chant_int    (repeat, _("Repeat"),
                   1, 100, 1, _("Repeat"))


#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE "noise-hurl.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input" , babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
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
  gfloat     *GEGL_ALIGNED in_pixel;
  gfloat     *GEGL_ALIGNED out_pixel;
  gfloat     *out_pix;
  gint        i, cnt;

  in_pixel  = in_buf;
  out_pixel = out_buf;
  out_pix   = out_pixel;

  for (i = 0; i < n_pixels * 4; i++)
    {
      *out_pix = *in_pixel;
      out_pix += 1;
      in_pixel += 1;
    }

  for (cnt = 0; cnt < o->repeat; cnt++)
    {
      gint x, y, n;

      x = roi->x;
      y = roi->y;
      n = 0;

      out_pix = out_pixel;

      for (i = 0; i < n_pixels; i++)
        {
          gfloat red, green, blue, alpha;

          red   = out_pix[0];
          green = out_pix[1];
          blue  = out_pix[2];
          alpha = out_pix[3];

          if (gegl_random_double_range (o->seed, x, y, 0, n++, 0.0, 100.0) <=
              o->pct_random)
            {
              red   = gegl_random_double_range (o->seed, x, y, 0, n++, 0.0, 1.0);
              green = gegl_random_double_range (o->seed, x, y, 0, n++, 0.0, 1.0);
              blue  = gegl_random_double_range (o->seed, x, y, 0, n++, 0.0, 1.0);
            }

          out_pix[0] = red;
          out_pix[1] = green;
          out_pix[2] = blue;
          out_pix[3] = alpha;

          out_pix += 4;

          x++;
          if (x >= roi->x + roi->width)
            {
              x = roi->x;
              y++;
            }
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
      "name",       "gegl:noise-hurl",
      "categories", "noise",
      "description", _("Completely randomize a fraction of pixels"),
      NULL);
}

#endif
