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

gegl_chant_double (alpha, _("Alpha"),    -G_MAXDOUBLE, G_MAXDOUBLE, 1.2, _(""))
gegl_chant_double (scale, _("Scale"),    -G_MAXDOUBLE, G_MAXDOUBLE, 1.8, _(""))
gegl_chant_double (zoff,  _("Z offset"), -G_MAXDOUBLE, G_MAXDOUBLE,  -1, _(""))
gegl_chant_double (seed,  _("Seed"),     -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, _(""))
gegl_chant_int    (n,     _("Iteration"), 0, 20, 3, _(""))

#else

#define GEGL_CHANT_TYPE_POINT_RENDER
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
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *out_pixel = out_buf;
  gint        x = roi->x; /* initial x                   */
  gint        y = roi->y; /*           and y coordinates */


  while (n_pixels--)
    {
      gfloat val;

      val = PerlinNoise3D ((double) (x)/50.0,
                           (double) (y)/50.0,
                           (double) o->zoff, o->alpha, o->scale,
                           o->n);
      *out_pixel = val * 0.5 + 0.5;
      out_pixel ++;

      /* update x and y coordinates */
      x++;
      if (x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
        }
    }
  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  point_render_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:perlin-noise";
  operation_class->categories  = "render";
  operation_class->description = _("Perlin noise generator.");

  operation_class->no_cache = TRUE;
  operation_class->get_cached_region = NULL;
}

#endif
