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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x,      -G_MAXDOUBLE, G_MAXDOUBLE,  0.0, "left most pixel coordinate")
gegl_chant_double (y,      -G_MAXDOUBLE, G_MAXDOUBLE,  0.0, "top pixel coordinate")
gegl_chant_double (width,  -G_MAXDOUBLE, G_MAXDOUBLE, 10.0, "width in pixels")
gegl_chant_double (height, -G_MAXDOUBLE, G_MAXDOUBLE, 10.0, "height in pixels")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            crop
#define GEGL_CHANT_SELF            "crop.c"
#define GEGL_CHANT_DESCRIPTION     "crops the image, can be used to rectangulary" \
                              "clip buffers, as well as specifying what " \
                              "portion of a composition to render to file"
#define GEGL_CHANT_CATEGORIES      "geometry"
#define GEGL_CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"


#include <stdio.h>

int gegl_chant_foo = 0;

/* Actual image processing code
 ************************************************************************/
static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationFilter    *filter = GEGL_OPERATION_FILTER(operation);
  GeglBuffer  *input  = filter->input;
  ChantInstance *crop = GEGL_CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (filter->output)
    g_object_unref (filter->output);

  g_assert (input);
  g_assert (gegl_buffer_get_format (input));

  filter->output = g_object_new (GEGL_TYPE_BUFFER,
                                 "source",       input,
                                 "x",     (int)crop->x,
                                 "y",     (int)crop->y,
                                 "width",  (int)crop->width,
                                 "height", (int)crop->height,
                                 NULL);
  crop = NULL;
  return  TRUE;
}

static GeglRect
get_defined_region (GeglOperation *operation)
{
  GeglRect result;
  ChantInstance  *op_crop = (ChantInstance*)(operation);
  GeglRect *in_rect = gegl_operation_get_have_rect (operation, "input");
  if (!in_rect)
    return result;

  gegl_operation_set_have_rect (operation,
     op_crop->x, op_crop->y,
     op_crop->width, op_crop->height);
  result.x=op_crop->x;
  result.y=op_crop->y;
  result.w=op_crop->width;
  result.h=op_crop->height;
  return result;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  GeglRect *requested    = gegl_operation_need_rect (self);

  gegl_operation_set_need_rect (self, "input",
     requested->x,
     requested->y,
     requested->w, requested->h);
  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
