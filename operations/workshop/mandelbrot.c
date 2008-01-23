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

gegl_chant_double(real, -200.0, 200.0, -1.77, "imaginary coordinate")
gegl_chant_double(img,  -200.0, 200.0, 0.0, "real coordinate")
gegl_chant_double(level, -200.0, 200.0, 3.5, "")
gegl_chant_int (maxiter, 0, 512, 128, "maximum number of iterations")

#else

#define GEGL_CHANT_NAME           mandelbrot
#define GEGL_CHANT_SELF           "mandelbrot.c"
#define GEGL_CHANT_DESCRIPTION    "Mandelbrot renderer."
#define GEGL_CHANT_CATEGORIES     "render"
#define GEGL_CHANT_PREPARE
#define GEGL_CHANT_CLASS_INIT

#define GEGL_CHANT_SOURCE

#include "gegl-old-chant.h"

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

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);

  {
    gfloat *buf;
    gint pxsize;

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

    gegl_buffer_set (output, NULL, babl_format ("Y float"), buf,
                     GEGL_AUTO_ROWSTRIDE);
    g_free (buf);
  }

  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-256,-256, 1024, 1024};
  return result;
}

static void class_init (GeglOperationClass *klass)
{
  klass->adjust_result_region = NULL;
}

#endif
