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
 * Copyright 2011 Audrey Mandet <mandet.audrey@gmail.com>
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com), Stephen Norris
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (x, _("Center X"), 0.5)
    ui_range   (0.0, 1.0)
    ui_meta    ("unit", "relative-coordinate")
    ui_meta    ("axis", "x")

property_double (y, _("Center Y"), 0.5)
    ui_range   (0.0, 1.0)
    ui_meta    ("unit", "relative-coordinate")
    ui_meta    ("axis", "y")

property_double (amplitude, _("Amplitude"), 25.0)
    description(_("Amplitude of the ripple"))
    value_range (0.0, 1000.0)

property_double (period, _("Period"), 200.0)
    description(_("Period (wavelength) of the ripple"))
    value_range (0.1, 1000.0)

property_double (phi, _("Phase shift"), 0)
    value_range (-1, 1)

property_double (aspect, _("Aspect ratio"), 1.0)
    value_range (0.1, 10.0)  /* XXX: needs better ui type */

property_enum (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_CUBIC)
    description(_("Mathematical method for reconstructing pixel values"))

property_boolean (clamp, _("Clamp deformation"), FALSE)
    description(_("Limit deformation in the image area."))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE waves.c
#define GEGL_OP_NAME     waves

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o       = GEGL_PROPERTIES (operation);

  op_area->left   = o->amplitude;
  op_area->right  = o->amplitude;
  op_area->top    = o->amplitude;
  op_area->bottom = o->amplitude;

  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o       = GEGL_PROPERTIES (operation);
  GeglSampler        *sampler = gegl_buffer_sampler_new_at_level (input,
                                                         babl_format ("RGBA float"),
                                                         o->sampler_type,
                                                         level);
  GeglRectangle      *in_extent = gegl_operation_source_get_bounding_box (operation, "input");
  GeglBufferIterator *iter;

  GeglAbyssPolicy abyss = o->clamp ? GEGL_ABYSS_CLAMP : GEGL_ABYSS_NONE;

  gdouble px_x = gegl_coordinate_relative_to_pixel (o->x, in_extent->width);
  gdouble px_y = gegl_coordinate_relative_to_pixel (o->y, in_extent->height);

  gdouble scalex;
  gdouble scaley;

  if (o->aspect > 1.0)
    {
      scalex = 1.0;
      scaley = o->aspect;
    }
  else if (o->aspect < 1.0)
    {
      scalex = 1.0 / o->aspect;
      scaley = 1.0;
    }
  else
    {
      scalex = 1.0;
      scaley = 1.0;
    }

  iter = gegl_buffer_iterator_new (output, result, 0, babl_format ("RGBA float"),
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gint x = result->x;
      gint y = result->y;
      gfloat *out_pixel = iter->data[0];

      for (y = iter->roi[0].y; y < iter->roi[0].y + iter->roi[0].height; ++y)
        for (x = iter->roi[0].x; x < iter->roi[0].x + iter->roi[0].width; ++x)
          {
            gdouble radius;
            gdouble shift;
            gdouble dx;
            gdouble dy;
            gdouble ux;
            gdouble uy;

            dx = (x - px_x) * scalex;
            dy = (y - px_y) * scaley;

            if (!dx && !dy)
              radius = 0.000001;
            else
              radius = sqrt (dx * dx + dy * dy);

            shift = o->amplitude * sin (2.0 * G_PI * radius / o->period +
                                        2.0 * G_PI * o->phi);

            ux = dx / radius;
            uy = dy / radius;

            gegl_sampler_get (sampler,
                              x + (shift + ux) / scalex,
                              y + (shift + uy) / scaley,
                              NULL,
                              out_pixel,
                              abyss);

            out_pixel += 4;
          }
    }

  g_object_unref (sampler);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:waves",
    "title",              _("Waves"),
    "categories",         "distort",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description", _("Distort the image with waves"),
    NULL);
}

#endif
