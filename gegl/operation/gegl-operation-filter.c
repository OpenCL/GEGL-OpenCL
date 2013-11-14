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

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation-filter.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"

static gboolean gegl_operation_filter_process
                                      (GeglOperation        *operation,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level);

static void     attach                 (GeglOperation *operation);
static GeglNode *detect                (GeglOperation *operation,
                                        gint           x,
                                        gint           y);

static GeglRectangle get_bounding_box          (GeglOperation       *self);
static GeglRectangle get_required_for_output   (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *roi);


G_DEFINE_TYPE (GeglOperationFilter, gegl_operation_filter, GEGL_TYPE_OPERATION)


static void
gegl_operation_filter_class_init (GeglOperationFilterClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process                 = gegl_operation_filter_process;
  operation_class->attach                  = attach;
  operation_class->detect                  = detect;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_filter_init (GeglOperationFilter *self)
{
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READABLE |
                               GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}


static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation, x, y);
  return operation->node;
}

static gboolean
gegl_operation_filter_process (GeglOperation        *operation,
                               GeglOperationContext *context,
                               const gchar          *output_prop,
                               const GeglRectangle  *result,
                               gint                  level)
{
  GeglOperationFilterClass *klass;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  klass = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->process);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a filter", output_prop);
      return FALSE;
    }

  input  = gegl_operation_context_get_source (context, "input");
  output = gegl_operation_context_get_target (context, "output");

  success = klass->process (operation, input, output, result, level);

  if (input != NULL)
    g_object_unref (input);

  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}
