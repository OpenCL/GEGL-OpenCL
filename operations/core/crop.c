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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x,      "X",      -G_MAXFLOAT, G_MAXFLOAT,  0.0, "X")
gegl_chant_double (y,      "Y",      -G_MAXFLOAT, G_MAXFLOAT,  0.0, "Y")
gegl_chant_double (width,  "Width",  -G_MAXFLOAT, G_MAXFLOAT, 10.0, "Width")
gegl_chant_double (height, "Height", -G_MAXFLOAT, G_MAXFLOAT, 10.0, "Height")

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "crop.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"
#include <math.h>

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglNode   *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_operation_detect (input_node->operation,
                                  x - floor (o->x),
                                  y - floor (o->y));

  return operation->node;
}


static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantO    *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation, "input");
  GeglRectangle  result  = { 0, 0, 0, 0 };

  if (!in_rect)
    return result;

  result.x = o->x;
  result.y = o->y;
  result.width  = o->width;
  result.height = o->height;

  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {o->x, o->y, o->width, o->height};

  gegl_rectangle_intersect (&result, &result, input_region);

  return result;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  return *roi;
}

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *input;
  gboolean      success = FALSE;
  GeglRectangle extent = {o->x, o->y, o->width, o->height};

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a crop", output_prop);
      return FALSE;
    }

  input  = gegl_node_context_get_source (context, "input");

  if (input != NULL)
    {
      GeglBuffer *output;

      output = gegl_buffer_create_sub_buffer (input, &extent);
      gegl_node_context_set_object (context, "output", G_OBJECT (output));
      g_object_unref (input);
      success = TRUE;
    }
  else
    {
      if (!g_object_get_data (G_OBJECT (operation->node), "graph"))
        g_warning ("%s got %s",
                   gegl_node_get_debug_name (operation->node),
                   input==NULL?"input==NULL":"");
    }

  return success;
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->process = process;
  operation_class->get_defined_region = get_defined_region;
  operation_class->detect = detect;
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->get_defined_region = get_defined_region;
  operation_class->compute_input_request = compute_input_request;

  operation_class->name        = "crop";
  operation_class->categories  = "core";
  operation_class->description = "Crop a buffer";

  operation_class->no_cache = TRUE;
}

#endif
