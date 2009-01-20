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


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (red,   _("Red"),   -10.0, 10.0, 0.5,  _("Amount of red"))
gegl_chant_double (green, _("Green"), -10.0, 10.0, 0.25, _("Amount of green"))
gegl_chant_double (blue,  _("Blue"),  -10.0, 10.0, 0.25, _("Amount of blue"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE      "mono-mixer.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  /* set the babl format this operation prefers to work on */
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat      red   = o->red;
  gfloat      green = o->green;
  gfloat      blue  = o->blue;
  gfloat     *in_buf;
  gfloat     *out_buf;

 if ((result->width > 0) && (result->height > 0))
 {
     gint num_pixels = result->width * result->height;
     gint i;
     gfloat *in_pixel, *out_pixel;

     in_buf = g_new (gfloat, 4 * num_pixels);
     out_buf = g_new (gfloat, 2 * num_pixels);

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

     gegl_buffer_set (output, result, babl_format ("YA float"), out_buf,
                      GEGL_AUTO_ROWSTRIDE);

     g_free (in_buf);
     g_free (out_buf);
 }

 return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:mono-mixer";
  operation_class->categories  = "color";
  operation_class->description = _("Monochrome channel mixer");
}

#endif
