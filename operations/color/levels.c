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

#if GEGL_CHANT_PROPERTIES
  gegl_chant_double (in_low, -1.0, 4.0, 0.0,
      "Input luminance level to become lowest output")
  gegl_chant_double (in_high, -1.0, 4.0, 1.0, "Input luminance level to become white.")
  gegl_chant_double (out_low, -1.0, 4.0, 0.0, "Lowest luminance level in output")
  gegl_chant_double (out_high, -1.0, 4.0, 1.0, "Highest luminance level in output")

#else

#define GEGL_CHANT_NAME         levels
#define GEGL_CHANT_SELF         "levels.c"

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_DESCRIPTION  "Remaps the intensity range of the image"

#define GEGL_CHANT_CATEGORIES   "color"

#include "gegl-old-chant.h"

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
  gfloat              in_range;
  gfloat              out_range;
  gfloat              in_offset;
  gfloat              out_offset;
  gfloat              scale;
  glong               i;

  self = GEGL_CHANT_OPERATION (op);
  pixel = in_buf;

  in_offset = self->in_low * 1.0;
  out_offset = self->out_low * 1.0;
  in_range = self->in_high-self->in_low;
  out_range = self->out_high-self->out_low;

  if (in_range == 0.0)
    in_range = 0.00000001;
  
  scale = out_range/in_range;

  for (i=0; i<n_pixels; i++)
    {
      int c;
      for (c=0;c<3;c++)
        pixel[c] = (pixel[c]- in_offset) * scale + out_offset;
      pixel += 4;
    }
  return TRUE;
}

#endif
