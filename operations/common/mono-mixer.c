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
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

gegl_property_double (red, _("Red"),
    "description", _("Amount of red"),
    "default", 0.5, "min", -10.0, "max", 10.0,
    "ui-min", -1.0, "ui-max", 1.0,
    NULL)

gegl_property_double (green, _("Green"),
    "description", _("Amount of green"),
    "default", 0.5, "min", -10.0, "max", 10.0,
    "ui-min", -1.0, "ui-max", 1.0,
    NULL)

gegl_property_double (blue, _("Blue"),
    "description", _("Amount of blue"),
    "default", 0.5, "min", -10.0, "max", 10.0,
    "ui-min", -1.0, "ui-max", 1.0,
    NULL)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_FILE "mono-mixer.c"

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat      red   = o->red;
  gfloat      green = o->green;
  gfloat      blue  = o->blue;
  gfloat     * GEGL_ALIGNED in_pixel;
  gfloat     * GEGL_ALIGNED out_pixel;
  glong       i;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = in_pixel[0] * red + in_pixel[1] * green + in_pixel[2] * blue;
      out_pixel[1] = in_pixel[3];
      in_pixel  += 4;
      out_pixel += 2;
    }
  return TRUE;
}

#include "opencl/mono-mixer.cl.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:mono-mixer",
    "categories",  "color",
    "description", _("Monochrome channel mixer"),
    "cl-source",   mono_mixer_cl_source,
    NULL);
}

#endif
