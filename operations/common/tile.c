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
 * Copyright 2012 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (offset_x, _("Horizontal offset"), 0)
    ui_range (0, 1024)
    ui_meta  ("unit", "pixel-coordinate")
    ui_meta  ("axis", "x")

property_int (offset_y, _("Vertical offset"), 0)
    ui_range (0, 1024)
    ui_meta  ("unit", "pixel-coordinate")
    ui_meta  ("axis", "y")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     tile
#define GEGL_OP_C_SOURCE tile.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *format;

  format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle *rect = gegl_operation_source_get_bounding_box (operation, input_pad);

  if (rect)
    {
      return *rect;
    }
  else
    {
      GeglRectangle empty = {0,};
      return empty;
    }
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_roi)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_buffer_set_pattern (output, result, input,
                           o->offset_x,
                           o->offset_y);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop'>"
    "  <params>"
    "    <param name='width'>200.0</param>"
    "    <param name='height'>200.0</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:tile'>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-aux.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                      = process;
  operation_class->prepare                   = prepare;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;

  gegl_operation_class_set_keys (operation_class,
      "name",                 "gegl:tile",
      "title",                _("Tile"),
      "categories",           "tile",
      "position-dependent",   "true",
      "reference-hash",       "166a4c955bb10d0a8472a2d8892baf39",
      "reference-composition", composition,
      "description",
      _("Infinitely repeats the input image."),
      NULL);
}
#endif
