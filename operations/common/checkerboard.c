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
#include <stdlib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int_ui   (x, _("Width"),
                     1, G_MAXINT, 16, 1, 256, 1.5,
                     _("Horizontal width of cells pixels"))

gegl_chant_int_ui   (y, _("Height"),
                     1, G_MAXINT, 16, 1, 256, 1.5,
                     _("Vertical width of cells in pixels"))

gegl_chant_int_ui   (x_offset, _("X offset"),
                     -G_MAXINT, G_MAXINT, 0, -10, 10, 1.0,
                     _("Horizontal offset (from origin) for start of grid"))

gegl_chant_int_ui   (y_offset, _("Y offset"),
                     -G_MAXINT, G_MAXINT,  0, -10, 10, 1.0,
                     _("Vertical offset (from origin) for start of grid"))

gegl_chant_color    (color1, _("Color"),
                     "black",
                     _("One of the cell colors (defaults to 'black')"))

gegl_chant_color    (color2, _("Other color"),
                     "white",
                     _("The other cell color (defaults to 'white')"))

gegl_chant_format (format, _("Babl Format"),
                   _("The babl format of the output"))

#else

#define GEGL_CHANT_TYPE_POINT_RENDER
#define GEGL_CHANT_C_FILE "checkerboard.c"

#include "gegl-chant.h"
#include <gegl-utils.h>

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->format)
    gegl_operation_set_format (operation, "output", o->format);
  else
    gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

#define TILE_INDEX(coordinate,stride) \
  (((coordinate) >= 0)?\
      (coordinate) / (stride):\
      ((((coordinate) + 1) /(stride)) - 1))

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint        pixel_size = babl_format_get_bytes_per_pixel (out_format);
  guchar     *out_pixel = out_buf;
  void       *color1 = alloca(pixel_size);
  void       *color2 = alloca(pixel_size);
  gint        y;
  const gint  x_min = roi->x - o->x_offset;
  const gint  y_min = roi->y - o->y_offset;
  const gint  x_max = roi->x + roi->width - o->x_offset;
  const gint  y_max = roi->y + roi->height - o->y_offset;

  const gint  square_width  = o->x;
  const gint  square_height = o->y;

  gegl_color_get_pixel (o->color1, out_format, color1);
  gegl_color_get_pixel (o->color2, out_format, color2);

  for (y = y_min; y < y_max; y++)
    {
      gint  x = x_min;
      void *cur_color;

      /* Figure out which box we're in */
      gint tilex = TILE_INDEX (x, square_width);
      gint tiley = TILE_INDEX (y, square_height);
      if ((tilex + tiley) % 2 == 0)
        cur_color = color1;
      else
        cur_color = color2;

      while (x < x_max)
        {
          /* Figure out how long this stripe is */
          gint count;
          gint stripe_end = (TILE_INDEX (x, square_width) + 1) * square_width;
               stripe_end = stripe_end > x_max ? x_max : stripe_end;

          count = stripe_end - x;

          gegl_memset_pattern (out_pixel, cur_color, pixel_size, count);
          out_pixel += count * pixel_size;
          x = stripe_end;

          if (cur_color == color1)
            cur_color = color2;
          else
            cur_color = color1;
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

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:checkerboard",
    "categories",  "render",
    "description", _("Create a checkerboard pattern"),
    NULL);
}

#endif
