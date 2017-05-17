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
#include <math.h>


#ifdef GEGL_PROPERTIES
property_boolean (preserve_luminosity, _("Preserve luminosity"), FALSE)

property_double (red, _("Red Channel Multiplier"), 0.333)
    value_range (-5.0, 5.0)
    ui_range    (-2.0, 2.0)

property_double (green, _("Green Channel Multiplier"), 0.333)
    value_range (-5.0, 5.0)
    ui_range    (-2.0, 2.0)

property_double (blue, _("Blue Channel Multiplier"), 0.333)
    value_range (-5.0, 5.0)
    ui_range    (-2.0, 2.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     mono_mixer
#define GEGL_OP_C_SOURCE mono-mixer.c

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
  GeglProperties *o       = GEGL_PROPERTIES (op);
  gfloat      red         = o->red;
  gfloat      green       = o->green;
  gfloat      blue        = o->blue;
  gboolean    normalize   = o->preserve_luminosity;
  gfloat      norm_factor = 1.0;
  gfloat     * GEGL_ALIGNED in_pixel;
  gfloat     * GEGL_ALIGNED out_pixel;
  glong       i;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  if (normalize)
   {
     gdouble sum = red + green + blue;

     if (sum == 0.0)
       norm_factor = 1.0;
     else
       norm_factor = fabs (1 / sum);
   }

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] * red +
                      in_pixel[1] * green +
                      in_pixel[2] * blue) * norm_factor;
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
    "title",       _("Mono Mixer"),
    "categories",  "color",
    "reference-hash", "bda2471708bff38f7d7bc5e701ab5220",
    "description", _("Monochrome channel mixer"),
    "cl-source",   mono_mixer_cl_source,
    NULL);
}

#endif
