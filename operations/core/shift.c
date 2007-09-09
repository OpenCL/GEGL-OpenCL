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
 
gegl_chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE,  0.0,
                   "X coordinate of new origin.")
gegl_chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE,  0.0,
                   "Y coordinate of new origin.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            shift
#define GEGL_CHANT_DESCRIPTION     "Translate the buffer, an integer amount of pixels. (more efficient than the affine op translate which resamples the image.)"
#define GEGL_CHANT_SELF            "shift.c"
#define GEGL_CHANT_CATEGORIES      "transform"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <math.h>
#include <stdio.h>

int gegl_chant_foo = 0;

/*FIXME: check that all rounding done here makes sense */

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglBuffer    *input;
  GeglBuffer    *output;
  GeglChantOperation *translate = GEGL_CHANT_OPERATION (operation);

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));

  translate->x = floor (translate->x);
  translate->y = floor (translate->y);

  g_assert (input);

  /* XXX: this shifted buffer is a behavior not readily available in the
   *      pure C (non-gobject) part of the GeglBuffer API.
   */
  output = g_object_new (GEGL_TYPE_BUFFER,
                         "provider",    input,
                         "shift-x",     (int)-translate->x,
                         "shift-y",     (int)-translate->y,
                         "abyss-width", -1,  /* turn of abyss (relying on abyss
                                                of source) */
                         NULL);
  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
  return  TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle       result = {0,0,0,0};
  GeglChantOperation *op_shift = (GeglChantOperation*)(operation);
  GeglRectangle      *in_rect = gegl_operation_source_get_defined_region (operation, "input");
  if (!in_rect)
    return result;

  result = *in_rect;
  result.x += floor(op_shift->x);
  result.y += floor(op_shift->y);
 
  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation *operation,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  GeglChantOperation  *op_shift = (GeglChantOperation*)(operation);
 
  region.x += floor(op_shift->x);
  region.y += floor(op_shift->y);
  return region;
}

static GeglRectangle
compute_input_request (GeglOperation *operation,
                       const gchar   *input_pad,
                       GeglRectangle *roi)
{
  GeglChantOperation *op_shift = (GeglChantOperation*)(operation);
  GeglRectangle result = *roi;
  result.x -= floor(op_shift->x);
  result.y -= floor(op_shift->y);
  return result;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglChantOperation *shift = (GeglChantOperation*)(operation);
  GeglNode *input_node = gegl_operation_get_source_node (operation, "input");
  return gegl_operation_detect (input_node->operation, x-floor(shift->x), y-floor(shift->y));
  /*return gegl_operation_detect (input_node->operation, x+shift->x, y+shift->y);*/
}


static void class_init (GeglOperationClass *operation_class)
{
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->get_defined_region = get_defined_region;
  operation_class->compute_input_request = compute_input_request;
  operation_class->detect = detect;
  operation_class->no_cache = TRUE;
}

#endif
