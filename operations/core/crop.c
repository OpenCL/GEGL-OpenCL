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
#if GEGL_CHANT_PROPERTIES

gegl_chant_double (x,      -G_MAXDOUBLE, G_MAXDOUBLE,  0.0,
                   "Left-most pixel coordinate.")
gegl_chant_double (y,      -G_MAXDOUBLE, G_MAXDOUBLE,  0.0,
                   "Top-most pixel coordinate.")
gegl_chant_double (width,  -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                   "Width in pixels.")
gegl_chant_double (height, -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                   "Height in pixels.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            crop
#define GEGL_CHANT_SELF            "crop.c"
#define GEGL_CHANT_DESCRIPTION     "Crops the resulting image buffer computed by the sources of the crop operation, can be used to mask out unwanted data, the cropped out regions are interpreted as transparent black by nodes using regions outside the crop-area for compositing."
#define GEGL_CHANT_CATEGORIES      "misc"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"


#include <stdio.h>

int gegl_chant_foo = 0;

/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation *operation,
         GeglNodeContext *context,
         const GeglRectangle *result)
{
  GeglBuffer          *input;
  GeglBuffer          *output;
  GeglChantOperation  *crop = GEGL_CHANT_OPERATION (operation);
  GeglRectangle        extent = {crop->x, crop->y, crop->width, crop->height};
  
  input = gegl_node_context_get_source (context, "input");

  g_assert (input);

  output = gegl_buffer_create_sub_buffer (input, &extent);
  gegl_node_context_set_object (context, "output", G_OBJECT (output));
  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation *op_crop;
  GeglRectangle      *in_rect;
 
  op_crop = (GeglChantOperation*)(operation);
  in_rect = gegl_operation_source_get_defined_region (operation, "input");

  if (!in_rect)
    return result;

  result.x = op_crop->x;
  result.y = op_crop->y;
  result.width  = op_crop->width;
  result.height  = op_crop->height;

  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  GeglChantOperation  *op_crop = (GeglChantOperation*)(operation);
  GeglRectangle        crop_rect;

  gegl_rectangle_set (&crop_rect, op_crop->x, op_crop->y, op_crop->width, op_crop->height);
  gegl_rectangle_intersect (&crop_rect, &crop_rect, input_region);
 
  return crop_rect;
}

static void
class_init (GeglOperationClass *operation_class)
{
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->get_defined_region = get_defined_region;
  operation_class->no_cache = TRUE;
}

#endif
