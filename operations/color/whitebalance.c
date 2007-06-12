/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#if GEGL_CHANT_PROPERTIES
  gegl_chant_double (high_a_delta, -2.0, 2.0, 0.0, "")
  gegl_chant_double (high_b_delta, -2.0, 2.0, 0.0, "")
  gegl_chant_double (low_a_delta,  -2.0, 2.0, 0.0, "")
  gegl_chant_double (low_b_delta,  -2.0, 2.0, 0.0, "")
  gegl_chant_double (saturation,   -3.0, 3.0, 1.0, "")

#else

#define GEGL_CHANT_NAME         whitebalance
#define GEGL_CHANT_SELF         "whitebalance.c"

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_DESCRIPTION  "Allows changing the whitepoint and blackpoint of an image."

#define GEGL_CHANT_CATEGORIES   "color"
#define GEGL_CHANT_PREPARE

#include "gegl-chant.h"

static void prepare (GeglOperation *operation,
                     gpointer       context_id)
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
  GeglChantOperation *self;
  gfloat             *pixel;
  gfloat              a_base;
  gfloat              a_scale;
  gfloat              b_base;
  gfloat              b_scale;
  glong               i;

  self = GEGL_CHANT_OPERATION (op);
  pixel = in_buf;

  a_scale = (self->high_a_delta - self->low_a_delta);
  a_base = self->low_a_delta;
  b_scale = (self->high_b_delta - self->low_b_delta);
  b_base = self->low_b_delta;
  
  for (i=0; i<n_pixels; i++)
    {
      pixel[1] += pixel[0] * a_scale + a_base;
      pixel[2] += pixel[0] * b_scale + b_base;
      pixel[1] = pixel[1] * self->saturation;
      pixel[2] = pixel[2] * self->saturation;
      pixel += 4;
    }
  return TRUE;
}

#endif
