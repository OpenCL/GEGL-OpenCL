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
 * Copyright 2017 Red Hat, Inc.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (black, _("Black"), 0.0)
    description (_("Adjust the black level"))
    value_range (-0.1, 0.1)

property_double (exposure, _("Exposure"), 0.0)
    description (_("Adjust the exposure correction"))
    value_range (-3.0, 3.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     dt_exposure
#define GEGL_OP_C_SOURCE dt-exposure.c

#include "gegl-op.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  const Babl *format;

  format = babl_format ("R'G'B'A float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat black = (gfloat) o->black;
  gfloat diff;
  gfloat exposure_negated = (gfloat) -o->exposure;
  gfloat scale;
  gfloat white;
  glong i;

  white = exp2f (exposure_negated);
  diff = MAX (white - black, 0.01);
  scale = 1.0f / diff;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = (in[0] - black) * scale;
      out[1] = (in[1] - black) * scale;
      out[2] = (in[2] - black) * scale;
      out[3] = in[3];

      in += 4;
      out += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_filter_class =
    GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  point_filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "dt:exposure",
    "title",       _("Exposure"),
    "categories",  "color",
    "description", _("An implementation of Darktable's exposure filter."),
    NULL);
}

#endif
