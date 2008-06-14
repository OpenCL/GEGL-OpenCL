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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_vector (vector,   "Vector",
                             "A GeglVector representing the path of the stroke")
gegl_chant_color  (color,    "Color",      "rgba(0.1,0.2,0.3,1.0)",
                             "Color of paint to use")
gegl_chant_double (linewidth,"Linewidth",  0.0, 100.0, 3.0,
                             "width of stroke")
gegl_chant_double (hardness, "Hardness",   0.0, 1.0, 0.7,
                             "hardness of brush, 0.0 for soft brush 1.0 for hard brush.")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE "stroke.c"

#include "gegl-plugin.h"

/* the vector api isn't public yet */
#include "property-types/gegl-vector.h"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  gdouble        x0, x1, y0, y1;

  gegl_vector_get_bounds (o->vector, &x0, &x1, &y0, &y1);
  defined.x      = x0 - o->linewidth;
  defined.y      = y0 - o->linewidth;
  defined.width  = x1 - x0 + o->linewidth * 2;
  defined.height = y1 - y0 + o->linewidth * 2;

  return defined;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle box = get_bounding_box (operation);

  gegl_buffer_clear (output, &box);
  g_object_set_data (operation, "vector-radius", GINT_TO_POINTER((gint)(o->linewidth+1)/2));
  gegl_vector_stroke (output, o->vector, o->color, o->linewidth, o->hardness);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;

  operation_class->name        = "stroke";
  operation_class->categories  = "render";
  operation_class->description = "Renders a brush stroke";
  operation_class->get_cached_region = NULL;
}


#endif
