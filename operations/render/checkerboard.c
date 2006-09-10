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

gegl_chant_int   (x,        -G_MAXINT, G_MAXINT, 16, "")
gegl_chant_int   (y,        -G_MAXINT, G_MAXINT, 16, "")
gegl_chant_int   (x_offset, -G_MAXINT, G_MAXINT,  0, "")
gegl_chant_int   (y_offset, -G_MAXINT, G_MAXINT,  0, "")
gegl_chant_color (color1,    "black",                "")
gegl_chant_color (color2,    "white",                "")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           checkerboard
#define GEGL_CHANT_DESCRIPTION    "Checkerboard renderer. Colors defaults to black and white. "

#define GEGL_CHANT_SELF           "checkerboard.c"
#define GEGL_CHANT_CATEGORIES     "sources:render"
#include "gegl-chant.h"

#define REMAINDER(dividend, divisor)                     \
    ((dividend) < 0 ?                                    \
       (divisor) - 1 - ((-(dividend) - 1) % (divisor)) : \
       (dividend) % (divisor))

static gboolean
process (GeglOperation *operation)
{
  GeglRect  *need;
  GeglOperationSource  *op_source = GEGL_OPERATION_SOURCE(operation);
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  need = gegl_operation_get_requested_region (operation);
  {
    GeglRect *result = gegl_operation_result_rect (operation);
    gfloat *buf;
    gfloat color1[4];
    gfloat color2[4];

    gegl_color_get_rgba (self->color1,
                         &color1[0],
                         &color1[1],
                         &color1[2],
                         &color1[3]);

    gegl_color_get_rgba (self->color2,
                         &color2[0],
                         &color2[1],
                         &color2[2],
                         &color2[3]);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "format",
                        babl_format ("RGBA float"),
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
    buf = g_malloc (gegl_buffer_pixels (op_source->output) * gegl_buffer_px_size (op_source->output));
      {
        gfloat *dst=buf;
        gint y;
        for (y=0; y < result->h; y++)
          {
            gint x;
            for (x=0; x < result->w; x++)
              {
                gfloat *rgba_color;
                gint nx,ny;

                nx = ((x + result->x + self->x_offset + 100000 * self->x)/self->x) ;
                ny = ((y + result->y + self->y_offset + 100000 * self->y)/self->y) ;

                rgba_color = (nx+ny) % 2 == 0 ? color1 : color2;

                memcpy(dst, rgba_color, 4*sizeof(gfloat));
                dst += 4;
              }
          }
      }
    gegl_buffer_set (op_source->output, buf);
    g_free (buf);
  }
  return  TRUE;
}

static GeglRect 
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {-10000000,-10000000, 20000000, 20000000};
  return result;
}

#endif
