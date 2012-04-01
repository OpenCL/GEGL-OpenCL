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

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double(real, _("Real"),      -200.0, 200.0, -1.77,
                  _("Real coordinate"))
gegl_chant_double(img,  _("Imaginary"), -200.0, 200.0, 0.0,
                  _("Imaginary coordinate"))
gegl_chant_double(level, _("Level"), -200.0, 200.0, 3.5,
                  _("Water level"))
gegl_chant_int (maxiter, _("Iterations"), 0, 512, 128,
                _("Maximum number of iterations"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE           "mandelbrot.c"

#include "gegl-chant.h"

static gfloat
mandel_calc(GeglChantO *o, gfloat x, gfloat y)
{
  gfloat fViewRectReal = o->real;
  gfloat fViewRectImg  = o->img;
  gfloat fMagLevel     = o->level;

  gfloat fCReal = fViewRectReal + x * fMagLevel;
  gfloat fCImg  = fViewRectImg + y * fMagLevel;
  gfloat fZReal = fCReal;
  gfloat fZImg  = fCImg;

        gint n;

        for (n=0;n<o->maxiter;n++)
          {
                gfloat fZRealSquared = fZReal * fZReal;
                gfloat fZImgSquared = fZImg * fZImg;

                if (fZRealSquared + fZImgSquared > 4)
                        return 1.0*n/(o->maxiter);

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

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {-256,-256, 1024, 1024};
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *buf;
  gint        pxsize;

  g_object_get (output, "px-size", &pxsize, NULL);

  buf = g_malloc (result->width * result->height * pxsize);

    {
      gfloat *dst = buf;
      gint y;
      for (y=0; y < result->height; y++)
        {
          gint x;
          for (x=0; x < result->width ; x++)
            {
              gfloat value;
              gfloat nx,ny;

              nx = (x + result->x);
              ny = (y + result->y);

              nx = (nx/512);
              ny = (ny/512);

              value = mandel_calc (o, nx, ny);

              *dst++ = value;
            }
        }
    }

  gegl_buffer_set (output, NULL, 0, babl_format ("Y float"), buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (buf);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:mandelbrot",
    "categories"  , "render",
    "description" , _("Mandelbrot set renderer"),
    NULL);
}

#endif
