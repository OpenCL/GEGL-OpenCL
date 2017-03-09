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
 * This operation is a port of the GIMP Engrave plug-in
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Eiichi Takamori
 * Copyright (C) 1996, 1997 Torsten Martinsen
 * 
 * GEGL Port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (row_height, _("Height"), 10)
    description (_("Resolution in pixels"))
    value_range (2, 16)
    ui_range    (2, 16)

property_boolean (limit, _("Limit line width"), FALSE)
    description (_("Limit line width"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     engrave
#define GEGL_OP_C_SOURCE engrave.c

#include "gegl-op.h"

static void
engrave_row (gfloat        *in_row,
             GeglRectangle *in_rect,
             gfloat        *out_row,
             GeglRectangle *out_rect,
             gboolean      limit)
{
  gfloat average, v;
  gint   x, y, y_offset, real_y, inten;

  y_offset = in_rect->height - out_rect->height;

  for (x = 0; x < in_rect->width; x++)
    {
      average = 0.0f;

      for (y = 0; y < in_rect->height; y++)
        {
          average += in_row[(x + y * in_rect->width) * 2];
        }
      
      inten = average; 

      for (y = 0; y < out_rect->height; y++)
        {
          real_y = y;

          if (in_rect->y != out_rect->y)
            real_y += y_offset;

          v = inten > real_y ? 1.0f : 0.0f;

          if (limit)
            {
              if (real_y == 0)
                v = 1.0f;
              else if (real_y == in_rect->height - 1)
                v = 0.0f;
            }

          out_row[(x + y * out_rect->width) * 2] = v;

          /* restore alpha */
          out_row[(x + y * out_rect->width) * 2 + 1] = in_row[(x + real_y * in_rect->width) * 2 + 1];
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o = GEGL_PROPERTIES (operation);
  const Babl              *format = babl_format ("Y'A float");

  area->left = area->right  = 0;
  area->top  = area->bottom = o->row_height;

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
  {
    result = *in_rect;
  }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("Y'A float");

  GeglRectangle *whole_region;
  GeglRectangle in_rect;
  GeglRectangle out_rect;

  gint row_count;
  gint top_offset, bottom_offset;
  gint extended_roi_y, extended_roi_height;
  gint i;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  top_offset = roi->y % o->row_height;
  bottom_offset = (roi->y + roi->height) % o->row_height;

  extended_roi_y = roi->y - top_offset;
  extended_roi_height = roi->height + top_offset + (o->row_height - bottom_offset);

  row_count = extended_roi_height / o->row_height;

  for (i = 0; i < row_count; i++)
    {
      gfloat *in_buf;
      gfloat *out_buf;
      gint   in_y;

      in_y = extended_roi_y + i * o->row_height;

      gegl_rectangle_set (&in_rect, roi->x, in_y, roi->width, o->row_height);
      gegl_rectangle_intersect (&in_rect, &in_rect, whole_region);

      gegl_rectangle_set (&out_rect, roi->x, in_y, roi->width, o->row_height);
      gegl_rectangle_intersect (&out_rect, &out_rect, roi);

      in_buf = g_new (gfloat, in_rect.width * in_rect.height * 2);
      out_buf = g_new (gfloat, out_rect.width * out_rect.height * 2);

      gegl_buffer_get (input, &in_rect, 1.0, format, in_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      engrave_row (in_buf, &in_rect, out_buf, &out_rect, o->limit);

      gegl_buffer_set (output, &out_rect, 0, format, out_buf,
                       GEGL_AUTO_ROWSTRIDE);

      g_free (in_buf);
      g_free (out_buf);
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:engrave",
    "title",       _("Engrave"),
    "categories",  "distort",
    "license",     "GPL3+",
    "reference-hash", "2fce9ed1adb05a50b7042fb9b5879529",
    "description", _("Simulate an antique engraving"),
    NULL);
}

#endif
