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

gegl_chant_double (alpha, -G_MAXDOUBLE, G_MAXDOUBLE, 1.2, "")
gegl_chant_double (scale, -G_MAXDOUBLE, G_MAXDOUBLE, 1.8, "")
gegl_chant_double (zoff,  -G_MAXDOUBLE, G_MAXDOUBLE,  -1, "")
gegl_chant_double (seed,  -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "")
gegl_chant_double (n,     0, 20.0, 3.0, "")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "noise.c"

#include "gegl-chant.h"
#include "perlin/perlin.c"
#include "perlin/perlin.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
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
                                   (double) o->zoff, o->alpha, o->scale,
                                   o->n);
              *dst = val * 0.5 + 0.5;
              dst ++;
            }
        }
    }
  gegl_buffer_set (output, NULL, babl_format ("Y float"), buf,
                   GEGL_AUTO_ROWSTRIDE);
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

  operation_class->name        = "perlin-noise";
  operation_class->categories  = "render";
  operation_class->description = "Perlin noise generator.";

  operation_class->no_cache = TRUE;
  operation_class->adjust_result_region = NULL;
}

#endif
