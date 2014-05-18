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

gegl_property_int (x, _("Width"),
    "description", _("Horizontal width of cells pixels"),
    "default", 32, "min", 1, "max", G_MAXINT,
    "ui-min", 1, "ui-max", 128,
    "unit", "pixel-distance",
    "axis", "x",
    NULL)

gegl_property_int (y, _("Height"),
    "description", _("Vertical width of cells pixels"),
    "default", 32, "min", 1, "max", G_MAXINT,
    "ui-min", 1, "ui-max", 128,
    "unit", "pixel-distance",
    "axis", "y",
    NULL)

gegl_property_int (x_offset, _("Offset X"),
    "description", _("Horizontal offset (from origin) for start of grid"),
    "default", 0,
    "ui-min", -64, "ui-max", 64,
    "unit", "pixel-coordinate",
    "axis", "x",
    NULL)

gegl_property_int (y_offset, _("Offset Y"),
    "description", _("Vertical offset (from origin) for start of grid"),
    "default", 0,
    "ui-min", -64, "ui-max", 64,
    "unit", "pixel-coordinate",
    "axis", "y",
    NULL)

gegl_property_int (line_width, _("Line width"),
    "description", _("Width of grid lines in pixels"),
    "default", 4, "min", 0,
    "ui-min", 0, "ui-max", 16,
    "unit", "pixel-distance",
    "axis", "x",
    NULL)

gegl_property_int (line_height, _("Line height"),
    "description", _("Width of grid lines in pixels"),
    "default", 4, "min", 0,
    "ui-min", 0, "ui-max", 16,
    "unit", "pixel-distance",
    "axis", "y",
    NULL)

gegl_property_color (line_color, _("Color"),
    "description", _("Color of the grid lines"),
    "default", "black",
    "role",    "color-primary",
    NULL)

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_C_FILE       "grid.c"

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
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
  gfloat     *out_pixel = out_buf;
  gfloat      line_color[4];
  gint        x = roi->x; /* initial x         */
  gint        y = roi->y; /* and y coordinates */

  gegl_color_get_pixel (o->line_color, babl_format ("RGBA float"), line_color);

  while (n_pixels--)
    {
      gint nx,ny;

      nx = (x - o->x_offset) % o->x;
      ny = (y - o->y_offset) % o->y;
      /* handle case where % returns a negative number */
      nx += nx < 0 ? o->x : 0;
      ny += ny < 0 ? o->y : 0;

      if (nx < o->line_width || ny < o->line_height)
        {
          out_pixel[0]=line_color[0];
          out_pixel[1]=line_color[1];
          out_pixel[2]=line_color[2];
          out_pixel[3]=line_color[3];
        }
      else
        {
          out_pixel[0]=0.0f;
          out_pixel[1]=0.0f;
          out_pixel[2]=0.0f;
          out_pixel[3]=0.0f;
        }

      out_pixel += 4;

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

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:grid",
    "categories" , "render",
    "description", _("Grid renderer"),
    NULL);
}

#endif
