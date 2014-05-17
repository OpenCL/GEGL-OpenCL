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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 * Copyright 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 * Copyright 1997 Brian Degenhardt <bdegenha@ucsd.edu>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

gegl_enum_start (gegl_ripple_wave_type)
  gegl_enum_value (GEGL_RIPPLE_WAVE_TYPE_SINE,     "Sine")
  gegl_enum_value (GEGL_RIPPLE_WAVE_TYPE_SAWTOOTH, "Sawtooth")
gegl_enum_end (GeglRippleWaveType)

gegl_property_double (amplitude, _("Amplitude"),
    "default", 25.0, "min", 0.0, "max", 1000.0,
    "ui-min", 0.0, "ui-max", 1000.0, "ui-gamma", 2.0,
    NULL)

gegl_property_double (period, _("Period"),
    "default", 200.0, "min", 0.0, "max", 1000.0,
    "ui-min", 0.0, "ui-max", 1000.0, "ui-gamma", 1.5,
    NULL)

gegl_property_double (phi, _("Phase shift"),
    "min", -1.0, "max", 1.0,
    NULL)

gegl_property_double (angle, _("Angle"),
    "min", -180.0, "max", 180.0,
    "unit", "degree",
    NULL)

gegl_property_enum  (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type, 
    "default", GEGL_SAMPLER_CUBIC,
    NULL)

gegl_property_enum (wave_type, _("Wave type"),
    GeglRippleWaveType, gegl_ripple_wave_type,
    "default", GEGL_RIPPLE_WAVE_TYPE_SINE,
    NULL)

gegl_property_boolean (tileable, _("Tileable"),
    "description", _("Retain tilebility"),
    NULL)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_FILE "ripple.c"

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  GeglProperties              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_PROPERTIES (operation);

  op_area->left   = o->amplitude;
  op_area->right  = o->amplitude;
  op_area->top    = o->amplitude;
  op_area->bottom = o->amplitude;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o       = GEGL_PROPERTIES (operation);
  GeglSampler        *sampler = gegl_buffer_sampler_new (input,
                                                         babl_format ("RGBA float"),
                                                         o->sampler_type);
  GeglBufferIterator *iter;

  GeglAbyssPolicy abyss = o->tileable ? GEGL_ABYSS_LOOP : GEGL_ABYSS_NONE;

  iter = gegl_buffer_iterator_new (output, result, 0, babl_format ("RGBA float"),
                                   GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gint x = result->x;
      gint y = result->y;
      gfloat *out_pixel = iter->data[0];

      for (y = iter->roi[0].y; y < iter->roi[0].y + iter->roi[0].height; ++y)
        for (x = iter->roi[0].x; x < iter->roi[0].x + iter->roi[0].width; ++x)
          {
            gdouble shift;
            gdouble coordsx;
            gdouble coordsy;
            gdouble lambda;

            gdouble angle_rad = o->angle / 180.0 * G_PI;
            gdouble nx = x * cos (angle_rad) + y * sin (angle_rad);

            switch (o->wave_type)
              {
                case GEGL_RIPPLE_WAVE_TYPE_SAWTOOTH:
                  lambda = div (nx,o->period).rem - o->phi * o->period;
                  if (lambda < 0)
                    lambda += o->period;
                  shift = o->amplitude * (fabs (((lambda / o->period) * 4) - 2) - 1);
                  break;
                case GEGL_RIPPLE_WAVE_TYPE_SINE:
                default:
                  shift = o->amplitude * sin (2.0 * G_PI * nx / o->period + 2.0 * G_PI * o->phi);
                  break;
              }

            coordsx = x + shift * sin (angle_rad);
            coordsy = y + shift * cos (angle_rad);

            gegl_sampler_get (sampler,
                              coordsx,
                              coordsy,
                              NULL,
                              out_pixel,
                              abyss);

            out_pixel += 4;
          }
    }

  g_object_unref (sampler);

  return  TRUE;
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
    "name",        "gegl:ripple",
    "categories",  "distort",
    "description", _("Displace pixels in a ripple pattern"),
    NULL);
}

#endif
