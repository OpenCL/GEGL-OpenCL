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
#define GEGL_CHANT_CATEGORIES      "geometry"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"


#include <stdio.h>

int gegl_chant_foo = 0;

/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglOperationFilter *filter;
  GeglBuffer          *input;
  GeglChantOperation  *crop;
  
  crop   = GEGL_CHANT_OPERATION (operation);
  filter = GEGL_OPERATION_FILTER(operation);
  input  = filter->input;

  g_assert (input);
  g_assert (gegl_buffer_get_format (input));

  filter->output = g_object_new (GEGL_TYPE_BUFFER,
                                 "source",       input,
                                 "x",      (int)crop->x,
                                 "y",      (int)crop->y,
                                 "width",  (int)crop->width,
                                 "height", (int)crop->height,
                                 NULL);
  return  TRUE;
}

static GeglRect
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {0,0,0,0};
  GeglChantOperation  *op_crop;
  GeglRect            *in_rect;
 
  op_crop = (GeglChantOperation*)(operation);
  in_rect = gegl_operation_source_get_defined_region (operation, "input");

  if (!in_rect)
    return result;

  result.x = op_crop->x;
  result.y = op_crop->y;
  result.w = op_crop->width;
  result.h = op_crop->height;


  return result;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       dynamic_id)
{
  gegl_operation_set_source_region (self, dynamic_id, "input",
                                    gegl_operation_get_requested_region (self, dynamic_id));
  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
