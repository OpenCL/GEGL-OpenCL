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

gegl_chant_double (alpha, -G_MAXDOUBLE, G_MAXDOUBLE, 1.2, "")
gegl_chant_double (scale, -G_MAXDOUBLE, G_MAXDOUBLE, 1.8, "")
gegl_chant_double (zoff,  -G_MAXDOUBLE, G_MAXDOUBLE,  -1, "")
gegl_chant_double (seed,  -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "")
gegl_chant_double (n,     0, 20.0, 3.0, "")

#else

#define GEGL_CHANT_NAME           perlin_noise
#define GEGL_CHANT_SELF           "noise.c"
#define GEGL_CHANT_DESCRIPTION    "Perlin noise generator."
#define GEGL_CHANT_CATEGORIES      "render"

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_PREPARE
#define GEGL_CHANT_CLASS_INIT

#include "gegl-chant.h"
#include "perlin/perlin.c"
#include "perlin/perlin.h"

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  const GeglRectangle *need;
  GeglBuffer          *output = NULL;

  need = gegl_operation_get_requested_region (operation, context_id);

  {
    const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    gfloat              *buf;

    output = gegl_operation_get_target (operation, context_id, "output");
    buf = g_malloc (result->width * result->height * 4);
      {
        gfloat *dst=buf;
        gint y;
        for (y=0; y < result->height; y++)
          {
            gint x;
            for (x=0; x < result->width ; x++)
              {
                gfloat val;

                val = PerlinNoise3D ((double) (x + result->x)/50.0,
                                     (double) (y + result->y)/50.0,
                                     (double) self->zoff, self->alpha, self->scale,
                                     self->n);
                *dst = val * 0.5 + 0.5;
                dst ++;
              }
          }
      }
    gegl_buffer_set (output, NULL, babl_format ("Y float"), buf);
    g_free (buf);
  }
  return  TRUE;
}

static void
prepare (GeglOperation *operation,
         gpointer       context_id)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

static void class_init (GeglOperationClass *klass)
{
  klass->adjust_result_region = NULL;
  klass->no_cache = FALSE;
}

#endif
