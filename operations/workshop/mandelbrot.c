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

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           mandelbrot
#define GEGL_CHANT_DESCRIPTION    "Mandelbrot renderer."

#define GEGL_CHANT_SELF           "mandelbrot.c"
#define GEGL_CHANT_CATEGORIES     "render"
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
         gpointer       dynamic_id)
{
  GeglRect  *need;
  GeglOperationSource  *op_source = GEGL_OPERATION_SOURCE(operation);
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  need = gegl_operation_get_requested_region (operation, dynamic_id);
  {
    GeglRect *result = gegl_operation_result_rect (operation, dynamic_id);
    gfloat *buf;

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "format",
                        babl_format ("Y float"),
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
    gegl_buffer_set (op_source->output, NULL, buf, NULL);
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
