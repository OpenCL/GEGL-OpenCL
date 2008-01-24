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


/* Followed by this #if ... */
#ifdef GEGL_CHANT_PROPERTIES
/* ... are the properties of the filter, these are all scalar values (doubles),
 * the the parameters are:
 *                 property name,   min,   max, default, "description of property"   */

gegl_chant_double (contrast,   -5.0, 5.0, 1.0, "Range scale factor")
gegl_chant_double (brightness, -3.0, 3.0, 0.0, "Amount to increase brightness")

/* this will create the following structure for our use, and register the
 * property with the given ranges, default values and a comment for the
 * documentation/tooltip.
 */
#else
/* Following an else, is then the meta information for this operation */

#define GEGL_CHANT_TYPE_POINT_FILTER
/* This sets the super class we are deriving from, in this case from the class
 * point filter. With a point filter we only need to implement processing for a
 * linear buffer
 */

#define GEGL_CHANT_C_FILE       "brightness-contrast.c"
/* we need to specify the name of the source file for gegl-old-chant.height  to
 * do it's magic.
 */


/* gegl-chant, uses the properties defined at the top, and the configuration
 * in the preceding lines to generate a GObject plug-in.
 */
#include "gegl-chant.h"

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation *op,
         void          *in_buf,
         void          *out_buf,
         glong          n_pixels)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     *pixel;
  glong       i;

  pixel = in_buf;

  for (i=0; i<n_pixels; i++)
    {
      gint component;
      for (component=0; component<3; component++)
        {
          gfloat c = pixel[component];
          c = (c - 0.5) * o->contrast + o->brightness + 0.5;
          pixel[component] = c;
        }
      pixel += 4;
    }
  return TRUE;
}


static void prepare (GeglOperation *operation)
{
  /* set the babl format this operation prefers to work on */
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "brightness-contrast";
  operation_class->categories  = "color";
  operation_class->description = "Changes the light level and contrast.";
}

#endif /* closing #ifdef GEGL_CHANT_PROPERTIES ... else ... */
