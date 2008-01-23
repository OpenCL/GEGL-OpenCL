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
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 */

#if GEGL_CHANT_PROPERTIES

 gegl_chant_double (red, -10.0, 10.0, 0.5, "Amount of red")
 gegl_chant_double (green, -10.0, 10.0, 0.25, "Amount of green")
 gegl_chant_double (blue, -10.0, 10.0, 0.25, "Amount of blue")

#else


#define GEGL_CHANT_NAME         mono_mixer
#define GEGL_CHANT_SELF         "mono-mixer.c"
#define GEGL_CHANT_DESCRIPTION  "Monochrome channel mixer"
#define GEGL_CHANT_CATEGORIES   "color"

#define GEGL_CHANT_FILTER

#include "gegl-old-chant.h"

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
 GeglChantOperation  *self;
 gfloat              *in_buf;
 gfloat              *out_buf;
 gfloat               red, green, blue;

 self = GEGL_CHANT_OPERATION (operation);

 red = self->red;
 green = self->green;
 blue = self->blue;

 if ((result->width > 0) && (result->height > 0))
 {
     gint num_pixels = result->width * result->height;
     gint i;
     gfloat *in_pixel, *out_pixel;

     in_buf = g_malloc (4 * sizeof (gfloat) * num_pixels);
     out_buf = g_malloc (2 * sizeof(gfloat) * num_pixels);

     gegl_buffer_get (input, 1.0, result, babl_format ("RGBA float"), in_buf, GEGL_AUTO_ROWSTRIDE);

     in_pixel = in_buf;
     out_pixel = out_buf;
     for (i = 0; i < num_pixels; ++i)
     {
         out_pixel[0] = in_pixel[0] * red + in_pixel[1] * green + in_pixel[2] * blue;
         out_pixel[1] = in_pixel[3];

         in_pixel += 4;
         out_pixel += 2;
     }

     gegl_buffer_set (output, result, NULL, out_buf,
                      GEGL_AUTO_ROWSTRIDE);

     g_free (in_buf);
     g_free (out_buf);
 }

 return TRUE;
}

#endif
