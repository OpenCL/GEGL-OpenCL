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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (high_a_delta, "High a delta", -2.0, 2.0, 0.0, "")
gegl_chant_double (high_b_delta, "High b delta", -2.0, 2.0, 0.0, "")
gegl_chant_double (low_a_delta,  "Low a delta",  -2.0, 2.0, 0.0, "")
gegl_chant_double (low_b_delta,  "Low b delta",  -2.0, 2.0, 0.0, "")
gegl_chant_double (saturation,   "Saturation",   -3.0, 3.0, 1.0, "")

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "whitebalance.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("Y'CbCrA float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation *op,
         void          *in_buf,
         void          *out_buf,
         glong          n_pixels)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     *pixel;
  gfloat      a_base;
  gfloat      a_scale;
  gfloat      b_base;
  gfloat      b_scale;
  glong       i;

  pixel = in_buf;

  a_scale = (o->high_a_delta - o->low_a_delta);
  a_base = o->low_a_delta;
  b_scale = (o->high_b_delta - o->low_b_delta);
  b_base = o->low_b_delta;

  for (i=0; i<n_pixels; i++)
    {
      pixel[1] += pixel[0] * a_scale + a_base;
      pixel[2] += pixel[0] * b_scale + b_base;
      pixel[1] = pixel[1] * o->saturation;
      pixel[2] = pixel[2] * o->saturation;
      pixel += 4;
    }
  return TRUE;
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "whitebalance";
  operation_class->categories  = "color";
  operation_class->description =
        "Allows changing the whitepoint and blackpoint of an image.";
}

#endif
