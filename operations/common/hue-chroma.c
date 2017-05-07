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
 * Copyright 2017 Elle Stone <ellestone@ninedegreesbelow.com>
 *                Michael Natterer <mitch@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (hue, _("Hue"),  0.0)
   description  (_("Hue adjustment"))
   value_range  (-180.0, 180.0)

property_double (chroma, _("Chroma"), 0.0)
   description  (_("Chroma adjustment"))
   value_range  (-100.0, 100.0)

property_double (lightness, _("Lightness"), 0.0)
   description  (_("Lightness adjustment"))
   value_range  (-100.0, 100.0)

#else

#define GEGL_OP_POINT_FILTER

#define GEGL_OP_NAME     hue_chroma
#define GEGL_OP_C_SOURCE hue-chroma.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("CIE LCH(ab) alpha float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("CIE LCH(ab) alpha float"));
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
  gfloat         *GEGL_ALIGNED in_pixel;
  gfloat         *GEGL_ALIGNED out_pixel;
  gfloat          hue;
  gfloat          chroma;
  gfloat          lightness;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  hue       = o->hue;
  chroma    = o->chroma;
  lightness = o->lightness;

  while (n_pixels--)
    {
      out_pixel[0] = in_pixel[0] + lightness;
      out_pixel[1] = in_pixel[1] + chroma;
      out_pixel[2] = in_pixel[2] + hue;

      out_pixel[1] = CLAMP (out_pixel[1], 0, 200.0);

      out_pixel[3] = in_pixel[3];

      in_pixel  += 4;
      out_pixel += 4;
    }

  return TRUE;
}

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
      "name",       "gegl:hue-chroma",
      "title",      _("Hue-Chroma"),
      "categories", "color",
      "description", _("Adjust LCH Hue, Chroma, and Lightness"),
      NULL);
}

#endif
