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

gegl_chant_double(real, -200.0, 200.0, -1.77, "imaginary coordinate")
gegl_chant_double(img,  -200.0, 200.0, 0.0, "real coordinate")
gegl_chant_double(level, -200.0, 200.0, 3.5, "")
gegl_chant_int (maxiter, 0, 512, 128, "maximum number of iterations")

#else

#define GEGL_CHANT_NAME           mandelbrot
#define GEGL_CHANT_SELF           "mandelbrot.c"
#define GEGL_CHANT_DESCRIPTION    "Mandelbrot renderer."
#define GEGL_CHANT_CATEGORIES     "render"

#define GEGL_CHANT_SOURCE

#include "gegl-chant.h"

static gfloat mandel_calc(GeglChantOperation *self, gfloat x, gfloat y)
{
  gfloat fViewRectReal = self->real;
  gfloat fViewRectImg  = self->img;
  gfloat fMagLevel     = self->level;

  gfloat fCReal = fViewRectReal + x * fMagLevel;
  gfloat fCImg  = fViewRectImg + y * fMagLevel;
  gfloat fZReal = fCReal;
  gfloat fZImg  = fCImg;
    
        gint n;
    
        for (n=0;n<self->maxiter;n++)
          {
                gfloat fZRealSquared = fZReal * fZReal;
                gfloat fZImgSquared = fZImg * fZImg;

                if (fZRealSquared + fZImgSquared > 4)
                        return 1.0*n/(self->maxiter);

/*                -- z = z^2 + c*/
                fZImg = 2 * fZReal * fZImg + fCImg;
                fZReal = fZRealSquared - fZImgSquared + fCReal;
          }
        return 1.0;
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglRectangle      *need;
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer *output;

  need = gegl_operation_get_requested_region (operation, context_id);
  {
    GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    gfloat *buf;
    gint pxsize;

    output = gegl_buffer_new (result, babl_format ("Y float"));
    g_object_get (output, "px-size", &pxsize, NULL);

    buf = g_malloc (result->width * result->height * pxsize);
      {
        gfloat *dst=buf;
        gint y;
        for (y=0; y < result->height; y++)
          {
            gint x;
            for (x=0; x < result->width ; x++)
              {
                gfloat value;
                gfloat nx,ny;

                nx = (x + result->x) ;
                ny = (y + result->y) ;

                nx = (nx/512);
                ny = (ny/512);

                value = mandel_calc (self, nx, ny);

                *dst = value;
                dst ++;
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
