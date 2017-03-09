/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contains code originaly from GIMP tile-glass.c, copyright
 * Karl-Johan Andersson and Tim Copperfield.
 *
 * Glass Tile (sequential version) ported to GEGL:
 * Copyright 2014 Denis Knoepfle
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (tile_width, _("Tile Width"), 25)
   value_range (5, 500)
   ui_range    (5, 50)
   ui_meta     ("unit",   "pixel-distance")
   ui_meta     ("axis",   "x")

property_int (tile_height, _("Tile Height"), 25)
   value_range (5, 500)
   ui_range    (5, 50)
   ui_meta     ("unit",   "pixel-distance")
   ui_meta     ("axis",   "y")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     tile_glass
#define GEGL_OP_C_SOURCE tile-glass.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  const Babl *input_format = gegl_operation_get_source_format (operation, "input");
  const Babl *format;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  if (input_format == NULL || babl_format_has_alpha (input_format))
    format = babl_format ("R'G'B'A float");
  else
    format = babl_format ("R'G'B' float");

  area->left = area->right = o->tile_width - 1;
  area->top = area->bottom = o->tile_height - 1;

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *region;

  region = gegl_operation_source_get_bounding_box (operation, "input");

  if (region != NULL)
    return *region;
  else
    return *GEGL_RECTANGLE (0, 0, 0, 0);
}

static void
tile_glass (GeglBuffer          *src,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect, /* region of interest */
            const Babl          *format,
            gint                 tileWidth,
            gint                 tileHeight)
{
  gint components;
  gfloat *src_row_buf;
  gfloat *dst_row_buf;
  const GeglRectangle *extent;
  GeglRectangle src_bufrect, dst_bufrect;

  gint row, col, i;
  gint x1, y1, y2;
  gint dst_xoffs, src_x0, xright_abyss, src_rowwidth;

  gint xpixel1, xpixel2;
  gint ypixel2;
  gint xhalf, xoffs, xmiddle, xplus;
  gint yhalf, yoffs, ymiddle, yplus;

  extent = gegl_buffer_get_extent (dst);

  x1 = dst_rect->x;
  y1 = dst_rect->y;
  y2 = y1 + dst_rect->height;

  xhalf = tileWidth  / 2;
  yhalf = tileHeight / 2;

  xplus = tileWidth  % 2;
  yplus = tileHeight % 2;

  dst_xoffs = x1 % tileWidth + xplus;
  src_x0 = x1 - dst_xoffs;
  xright_abyss = 2 * ((x1 + dst_rect->width) % tileWidth);
  if (xright_abyss > tileWidth - 2)
    xright_abyss = tileWidth - 2;
  src_rowwidth = dst_xoffs + dst_rect->width + xright_abyss;

  yoffs = y1 % tileHeight;
  ymiddle = y1 - yoffs;
  if (yoffs >= yhalf)
    {
      ymiddle += tileHeight;
      yoffs -= tileHeight;
    }

  components = babl_format_get_n_components (format);

  src_row_buf = g_new (gfloat, src_rowwidth * components);
  dst_row_buf = g_new (gfloat, dst_rect->width * components);
  gegl_rectangle_set (&src_bufrect, src_x0, 0, src_rowwidth, 1);
  gegl_rectangle_set (&dst_bufrect, x1, 0, dst_rect->width, 1);

  /* loop through rows */
  for (row = y1; row < y2; ++row)
    {
      /* no need to clamp as abyss policy does that */
      ypixel2 = ymiddle + yoffs * 2;

      src_bufrect.y = ypixel2;
      gegl_buffer_get (src, &src_bufrect, 1.0, format, src_row_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      yoffs++;

      /* if current offset = half, do a displacement next time around */
      if (yoffs == yhalf)
        {
          ymiddle += tileHeight;
          yoffs = - (yhalf + yplus);
        }

      xoffs = x1 % tileWidth;
      xmiddle = x1 - xoffs;
      if (xoffs >= xhalf)
        {
          xmiddle += tileWidth;
          xoffs -= tileWidth;
        }

      /* loop through columns */
      for (col = 0; col < dst_rect->width; ++col)
        {
          xpixel1 = (xmiddle + xoffs - x1) * components;
          if (xmiddle + xoffs * 2 + dst_xoffs < extent->width)
            {
              xpixel2 = (xmiddle + xoffs * 2 - x1 + dst_xoffs) * components;
            }
          else
            {
              xpixel2 = (xmiddle + xoffs - x1 + dst_xoffs) * components;
            }

          for (i = 0; i < components; ++i)
            {
              dst_row_buf[xpixel1 + i] = src_row_buf[xpixel2 + i];
            }

          xoffs++;

          if (xoffs == xhalf)
            {
              xmiddle += tileWidth;
              xoffs = - (xhalf + xplus);
            }
        }

      /* write result row to dest */
      dst_bufrect.y = row;
      gegl_buffer_set (dst, &dst_bufrect, 0, format, dst_row_buf, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (src_row_buf);
  g_free (dst_row_buf);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "input");

  tile_glass (input, output, result, format, o->tile_width, o->tile_height);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  filter_class->process             = process;

  gegl_operation_class_set_keys (operation_class,
    "categories",         "artistic:map",
    "title",              _("Tile Glass"),
    "license",            "GPL3+",
    "name",               "gegl:tile-glass",
    "reference-hash",     "d949027ca089aed4d4bea2950a33c56f",
    "position-dependent", "true",
    "description", _("Simulate distortion caused by rectangular glass tiles"),
    NULL);
}

#endif
