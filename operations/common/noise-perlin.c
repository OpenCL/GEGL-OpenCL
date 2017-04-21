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


#ifdef GEGL_PROPERTIES

property_double (alpha, _("Alpha"), 1.2)
    ui_range    (0.0, 4.0)
property_double (scale, _("Scale"), 1.8)
    ui_range    (0.0, 20.0)

property_double (zoff,  _("Z offset"), -1)
    ui_range    (-1.0, 8.0)

property_int (n, _("Iterations"), 3)
  value_range     (0, 20)

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     noise_perlin
#define GEGL_OP_C_SOURCE noise-perlin.c

#include "gegl-op.h"
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
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
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
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  point_render_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->no_cache = TRUE;
  operation_class->get_cached_region = NULL;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:perlin-noise",
    "title",              _("Perlin Noise"),
    "categories",         "render",
    "reference-hash",     "78a43934ae5b69e48ed523a61bdea6c4",
    "position-dependent", "true",
    "description", _("Perlin noise generator"),
    NULL);

}

#endif
