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
 
gegl_chant_double (alpha, -G_MAXDOUBLE, G_MAXDOUBLE, 12.3, "")
gegl_chant_double (beta,  -G_MAXDOUBLE, G_MAXDOUBLE, 0.1, "")
gegl_chant_double (zoff,  -G_MAXDOUBLE, G_MAXDOUBLE,  -1, "")
gegl_chant_double (seed,  -G_MAXDOUBLE, G_MAXDOUBLE, 20.0, "")
gegl_chant_double (n,     0, 20.0, 6.0, "")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           perlin_noise
#define GEGL_CHANT_DESCRIPTION    "Perlin noise generator."

#define GEGL_CHANT_SELF           "noise.c"
#define GEGL_CHANT_CATEGORIES      "sources:render"
#include "gegl-chant.h"
#include "perlin/perlin.c"
#include "perlin/perlin.h"

static gboolean
process (GeglOperation *operation)
{
  GeglRect             *need;
  GeglOperationSource  *op_source = GEGL_OPERATION_SOURCE(operation);
  GeglChantOperation   *self      = GEGL_CHANT_OPERATION (operation);

  need = gegl_operation_get_requested_region (operation);

  {
    GeglRect *result = gegl_operation_result_rect (operation);
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
                gfloat val;

                val = PerlinNoise3D ((double) (x + result->x)/50.0,
                                     (double) (y + result->y)/50.0,
                                     (double) self->zoff, self->alpha, self->beta,
                                     self->n);
                *dst = val * 0.5 + 0.5;
                dst ++;
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
  GeglRect result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

#endif
