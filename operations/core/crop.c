/* This file is part of GEGL
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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (x,      _("X"),      0.0)
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-coordinate")
  ui_meta       ("axis", "x")

property_double (y,      _("Y"),      0.0)
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-coordinate")
  ui_meta       ("axis", "y")

property_double (width,  _("Width"),  10.0 )
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")

property_double (height, _("Height"), 10.0 )
  ui_range      (0.0, 1024.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")

property_boolean (reset_origin, _("Reset origin"), FALSE)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     crop
#define GEGL_OP_C_SOURCE crop.c

#include "gegl-op.h"
#include <math.h>

static void
gegl_crop_prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglNode *
gegl_crop_detect (GeglOperation *operation,
                  gint           x,
                  gint           y)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode   *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_node_detect (input_node,
                             x - floor (o->x),
                             y - floor (o->y));

  return operation->node;
}


static GeglRectangle
gegl_crop_get_bounding_box (GeglOperation *operation)
{
  GeglProperties      *o = GEGL_PROPERTIES (operation);
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglRectangle  result  = { 0, 0, 0, 0 };

  if (!in_rect)
    return result;

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, in_rect);

  return result;
}

static GeglRectangle
gegl_crop_get_invalidated_by_change (GeglOperation       *operation,
                                     const gchar         *input_pad,
                                     const GeglRectangle *input_region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result;

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, input_region);

  return result;
}

static GeglRectangle
gegl_crop_get_required_for_output (GeglOperation       *operation,
                                   const gchar         *input_pad,
                                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result;

  result.x      = o->x;
  result.y      = o->y;
  result.width  = o->width;
  result.height = o->height;

  gegl_rectangle_intersect (&result, &result, roi);
  return result;
}

static gboolean
gegl_crop_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer     *input;
  gboolean        success = FALSE;

  input = gegl_operation_context_get_source (context, "input");

  if (input)
    {
      GeglRectangle  extent;
      GeglBuffer    *output;

      extent = *GEGL_RECTANGLE (o->x, o->y,  o->width, o->height);

      /* The output buffer's extent must be a subset of the input buffer's
       * extent; otherwise, if the output buffer is reused for in-place output,
       * we might try to write to areas of the buffer that lie outside the
       * input buffer, erroneously discarding the data.
       */
      gegl_rectangle_intersect (&extent,
                                &extent, gegl_buffer_get_extent (input));

      output = gegl_buffer_create_sub_buffer (input, &extent);

      if (gegl_object_get_has_forked (G_OBJECT (input)))
        gegl_object_set_has_forked (G_OBJECT (output));

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      g_object_unref (input);
      success = TRUE;
    }
  else
    {
      g_warning ("%s got NULL input pad", gegl_node_get_operation (operation->node));
    }

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  gchar              *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop'>"
    "  <params>"
    "    <param name='x'>50</param>"
    "    <param name='y'>80</param>"
    "    <param name='width'>70</param>"
    "    <param name='height'>60</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->threaded                  = FALSE;
  operation_class->process                   = gegl_crop_process;
  operation_class->prepare                   = gegl_crop_prepare;
  operation_class->get_bounding_box          = gegl_crop_get_bounding_box;
  operation_class->detect                    = gegl_crop_detect;
  operation_class->get_invalidated_by_change = gegl_crop_get_invalidated_by_change;
  operation_class->get_required_for_output   = gegl_crop_get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:crop",
      "categories",  "core",
      "title",       _("Crop"),
      "description", _("Crop a buffer"),
      "reference-composition", composition,
      NULL);

  operation_class->no_cache = TRUE;
}

#endif
