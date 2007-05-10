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

gegl_chant_int   (x,        -G_MAXINT, G_MAXINT, 16, "Horizontal width of cells pixels.")
gegl_chant_int   (y,        -G_MAXINT, G_MAXINT, 16, "Vertical width of cells in pixels.")
gegl_chant_int   (x_offset, -G_MAXINT, G_MAXINT,  0, "Horizontal offset (from origin) for start of grid.")
gegl_chant_int   (y_offset, -G_MAXINT, G_MAXINT,  0, "Vertical offset (from origin) for start of grid.")
gegl_chant_color (color1,    "black",                "One of the cell colors (defaults to 'black')")
gegl_chant_color (color2,    "white",                "The other cell color (defaults to 'white')")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           checkerboard
#define GEGL_CHANT_DESCRIPTION    "Checkerboard renderer."

#define GEGL_CHANT_SELF           "checkerboard.c"
#define GEGL_CHANT_CATEGORIES     "render"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglRectangle      *need;
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer         *output = NULL;


  need = gegl_operation_get_requested_region (operation, context_id);
  {
    GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    gfloat        *buf;
    gfloat         color1[4];
    gfloat         color2[4];
    gint           pxsize;
    gint           n_pixels;


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

    output = g_object_new (GEGL_TYPE_BUFFER,
                           "format",
                           babl_format ("RGBA float"),
                           "x",      result->x,
                           "y",      result->y,
                           "width",  result->width ,
                           "height", result->height,
                           NULL);

    g_object_get (output, "px-size", &pxsize,
                          "pixels", &n_pixels,
                          NULL);
    buf = g_malloc (n_pixels * pxsize);
      {
        gfloat *dst=buf;
        gint y;
        for (y=0; y < result->height; y++)
          {
            gint x;
            for (x=0; x < result->width ; x++)
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
    gegl_buffer_set (output, NULL, NULL, buf);
    g_free (buf);
  }
  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
  return  TRUE;
}

static GeglRectangle 
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-10000000,-10000000, 20000000, 20000000};
  return result;
}

#endif
