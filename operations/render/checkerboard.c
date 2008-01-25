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

gegl_chant_int   (x,        -G_MAXINT, G_MAXINT, 16, "Horizontal width of cells pixels.")
gegl_chant_int   (y,        -G_MAXINT, G_MAXINT, 16, "Vertical width of cells in pixels.")
gegl_chant_int   (x_offset, -G_MAXINT, G_MAXINT,  0, "Horizontal offset (from origin) for start of grid.")
gegl_chant_int   (y_offset, -G_MAXINT, G_MAXINT,  0, "Vertical offset (from origin) for start of grid.")
gegl_chant_color (color1,    "black",                "One of the cell colors (defaults to 'black')")
gegl_chant_color (color2,    "white",                "The other cell color (defaults to 'white')")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "checkerboard.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *buf;
  gfloat      color1[4];
  gfloat      color2[4];
  gint        pxsize;
  gint        n_pixels;

  gegl_color_get_rgba (o->color1,
                       &color1[0],
                       &color1[1],
                       &color1[2],
                       &color1[3]);

  gegl_color_get_rgba (o->color2,
                       &color2[0],
                       &color2[1],
                       &color2[2],
                       &color2[3]);

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

              nx = ((x + result->x + o->x_offset + 100000 * o->x)/o->x) ;
              ny = ((y + result->y + o->y_offset + 100000 * o->y)/o->y) ;

              rgba_color = (nx+ny) % 2 == 0 ? color1 : color2;

              memcpy (dst, rgba_color, 4*sizeof(gfloat));
              dst += 4;
            }
        }
    }

  gegl_buffer_set (output, NULL, NULL, buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);

  return  TRUE;
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_defined_region = get_defined_region;
  operation_class->prepare = prepare;

  operation_class->name        = "checkerboard";
  operation_class->categories  = "render";
  operation_class->description = "Checkerboard renderer.";

  operation_class->no_cache = TRUE;
  operation_class->adjust_result_region = NULL;
}

#endif
