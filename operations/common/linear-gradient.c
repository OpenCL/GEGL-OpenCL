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
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (start_x, _("X1"), 25.0)
    ui_meta("unit", "pixel-coordinate")
    ui_meta("axis", "x")

property_double (start_y, _("Y1"), 25.0)
    ui_meta("unit", "pixel-coordinate")
    ui_meta("axis", "y")

property_double (end_x, _("X2"), 150.0)
    ui_meta("unit", "pixel-coordinate")
    ui_meta("axis", "x")

property_double (end_y, _("Y2"), 150.0)
    ui_meta ("unit", "pixel-coordinate")
    ui_meta ("axis", "y")

property_color (start_color, _("Start Color"), "black")
    description (_("The color at (x1, y1)"))
    ui_meta ("role", "color-primary")

property_color  (end_color, _("End Color"), "white")
    description (_("The color at (x2, y2)"))
    ui_meta     ("role", "color-secondary")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     linear_gradient
#define GEGL_OP_C_SOURCE linear-gradient.c

#include "gegl-op.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat         *out_pixel = out_buf;
  gfloat          color1[4], color2[4], length, dx, dy;

  dx = o->end_x - o->start_x;
  dy = o->end_y - o->start_y;

  length = dx * dx + dy * dy;

  if (GEGL_FLOAT_IS_ZERO (length))
    {
      memset (out_buf, 0, n_pixels * sizeof(float) * 4);
    }
  else
    {
      gfloat vec0 = dx / length;
      gfloat vec1 = dy / length;
      gint x, y;

      gegl_color_get_pixel (o->start_color, babl_format ("R'G'B'A float"), color1);
      gegl_color_get_pixel (o->end_color, babl_format ("R'G'B'A float"), color2);

      for (y = roi->y; y < roi->y + roi->height; ++y)
        {
          for (x = roi->x; x < roi->x + roi->width; ++x)
            {
              gint c;
              gfloat v = vec0 * (x - o->start_x) + vec1 * (y - o->start_y);

              if (v > 1.0f - GEGL_FLOAT_EPSILON)
                v = 1.0f;
              if (v < 0.0f + GEGL_FLOAT_EPSILON)
                v = 0.0f;

              for (c = 0; c < 4; c++)
                out_pixel[c] = color1[c] * v + color2[c] * (1.0f - v);

              out_pixel += 4;
            }
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

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:linear-gradient",
    "title",              _("Linear Gradient"),
    "categories",         "render:gradient",
    "reference-hash",     "4e09f5cc5644c4262e4f4195a5a8cb5c",
    "position-dependent", "true",
    "description" , _("Linear gradient renderer"),
    NULL);
}

#endif
