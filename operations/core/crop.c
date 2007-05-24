/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
         gpointer       context_id)
{
  GeglBuffer          *input;
  GeglBuffer          *output;
  GeglChantOperation  *crop;
  
  crop   = GEGL_CHANT_OPERATION (operation);
  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));


  g_assert (input);

  output = g_object_new (GEGL_TYPE_BUFFER,
                         "source",       input,
                         "x",      (int)crop->x,
                         "y",      (int)crop->y,
                         "width",  (int)crop->width,
                         "height", (int)crop->height,
                         NULL);
  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
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
compute_affected_region (GeglOperation *operation,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  GeglChantOperation  *op_crop = (GeglChantOperation*)(operation);
  GeglRectangle        crop_rect;

  gegl_rectangle_set (&crop_rect, op_crop->x, op_crop->y, op_crop->width, op_crop->height);
  gegl_rectangle_intersect (&crop_rect, &crop_rect, &region);
 
  return crop_rect;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->get_defined_region = get_defined_region;
}

#endif
