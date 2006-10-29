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
#define GEGL_CHANT_DESCRIPTION     "Translate the buffer, an integer amount of pixels. (This is a lower level operation than the affine 'translate' operation.)"
#define GEGL_CHANT_SELF            "shift.c"
#define GEGL_CHANT_CATEGORIES      "geometry"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"


#include <stdio.h>

int gegl_chant_foo = 0;
  
/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation *operation)
{
  GeglOperationFilter    *filter = GEGL_OPERATION_FILTER(operation);
  GeglBuffer    *input  = filter->input;
  GeglChantOperation *translate = (GeglChantOperation*)filter;

  g_assert (input);
  g_assert (gegl_buffer_get_format (input));

  filter->output = g_object_new (GEGL_TYPE_BUFFER,
                                 "source",       input,
                                 "shift-x",     (int)-translate->x,
                                 "shift-y",     (int)-translate->y,
                                 "abyss-width", -1,  /* turn of abyss (relying
                                                        on abyss of source) */
                                 NULL);
  translate = NULL;
  return  TRUE;
}

static GeglRect
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {0,0,0,0};
  GeglChantOperation  *op_shift = (GeglChantOperation*)(operation);
  GeglRect *in_rect = gegl_operation_source_get_defined_region (operation, "input");
  if (!in_rect)
    return result;

  result = *in_rect;
  result.x += op_shift->x;
  result.y += op_shift->y;
 
  return result;
}

static GeglRect
get_affected_region (GeglOperation *operation,
                     const gchar   *input_pad,
                     GeglRect       region)
{
  GeglChantOperation  *op_shift = (GeglChantOperation*)(operation);
 
  region.x += op_shift->x;
  region.y += op_shift->y;
  return region;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  GeglChantOperation  *op_shift = (GeglChantOperation*)(self);
  GeglRect rect = *gegl_operation_get_requested_region (self);

  rect.x -= op_shift->x;
  rect.y -= op_shift->y;

  gegl_operation_set_source_region (self, "input", &rect);
  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_affected_region = get_affected_region;
  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
