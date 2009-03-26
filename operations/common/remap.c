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


#ifdef GEGL_CHANT_PROPERTIES

#if 0
gegl_chant_string ("xlow",  "hm", _("low description"))
gegl_chant_string ("xhigh", "hm", _("high description"))
#endif

#else

#define GEGL_CHANT_TYPE_OPERATION
#define GEGL_CHANT_C_FILE       "remap.c"

#include "gegl-chant.h"
#include <math.h>
#include <string.h>


static void
attach (GeglOperation *operation)
{
  GObjectClass *object_class = G_OBJECT_GET_CLASS (operation);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "input"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "low"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "high"));
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode      *input_node;
  GeglOperation *input_operation;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    {
      g_object_get (input_node, "gegl-operation", &input_operation, NULL);
      return gegl_operation_detect (input_operation, x, y);
    }

  return operation->node;
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

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  return *input_region;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}

static gboolean
process (GeglOperation       *operation,
         GeglOperationContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglBuffer *input;
  GeglBuffer *low;
  GeglBuffer *high;
  GeglBuffer *output;
  gfloat     *buf;
  gfloat     *min;
  gfloat     *max;
  gint        pixels = result->width * result->height;
  gint        i;

  input = gegl_operation_context_get_source (context, "input");
  low = gegl_operation_context_get_source (context, "low");
  high = gegl_operation_context_get_source (context, "high");

  buf = g_new (gfloat, pixels * 4);
  min = g_new (gfloat, pixels * 3);
  max = g_new (gfloat, pixels * 3);

  gegl_buffer_get (input, 1.0, result, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_get (low,   1.0, result, babl_format ("RGB float"), min, GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_get (high,  1.0, result, babl_format ("RGB float"), max, GEGL_AUTO_ROWSTRIDE);

  output = gegl_operation_context_get_target (context, "output");

  for (i = 0; i < pixels; i++)
    {
      gint c;

      for (c = 0; c < 3; c++)
        {
          gfloat delta = max[i*3+c]-min[i*3+c];

          if (delta > 0.0001 || delta < -0.0001)
            {
              buf[i*4+c] = (buf[i*4+c]-min[i*3+c]) / delta;
            }
          /*else
            buf[i*4+c] = buf[i*4+c];*/
        }
    }

  gegl_buffer_set (output, result, babl_format ("RGBA float"), buf,
                   GEGL_AUTO_ROWSTRIDE);

  g_free (buf);
  g_free (min);
  g_free (max);

  g_object_unref (input);
  g_object_unref (high);
  g_object_unref (low);

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process = process;
  operation_class->attach  = attach;
  operation_class->prepare = prepare;
  operation_class->detect  = detect;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_required_for_output = get_required_for_output;

  operation_class->name        = "gegl:remap";
  operation_class->categories  = "color";
  operation_class->description =
        _("Linearly remap the R,G,B based on per pixel minimum and maximum"
          " values from the high/low input pads");
}

#endif
