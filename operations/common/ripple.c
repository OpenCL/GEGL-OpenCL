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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_ripple_wave_type)
  enum_value (GEGl_RIPPLE_WAVE_TYPE_SINE,     "Sine")
  enum_value (GEGl_RIPPLE_WAVE_TYPE_SAWTOOTH, "Sawtooth")
gegl_chant_register_enum_end (GeglRippleWaveType)

gegl_chant_double_ui (amplitude, _("Amplitude"),
                      0.0, 1000.0, 25.0, 0.0, 1000.0, 2.0,
                      _("Amplitude of the ripple"))

gegl_chant_double_ui (period, _("Period"),
                      0.0, 1000.0, 200.0, 0.0, 1000.0, 1.5,
                      _("Period of the ripple"))

gegl_chant_double    (phi, _("Phase shift"),
                      -1.0, 1.0, 0.0,
                      _("Phase shift"))

gegl_chant_double    (angle, _("Angle"),
                      -180.0, 180.0, 0.0,
                      _("Angle in degree"))

gegl_chant_enum      (sampler_type, _("Sampler"),
                      GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_CUBIC,
                      _("Sampler used internally"))

gegl_chant_enum      (wave_type, _("Wave type"),
                      GeglRippleWaveType, gegl_ripple_wave_type,
                      GEGl_RIPPLE_WAVE_TYPE_SINE,
                      _("Type of wave"))

gegl_chant_boolean   (tileable, _("Tileable"),
                      FALSE,
                      _("Retain tilebility"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "ripple.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

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
  GeglChantO         *o       = GEGL_CHANT_PROPERTIES (operation);
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
                case GEGl_RIPPLE_WAVE_TYPE_SAWTOOTH:
                  lambda = div (nx,o->period).rem - o->phi * o->period;
                  if (lambda < 0)
                    lambda += o->period;
                  shift = o->amplitude * (fabs (((lambda / o->period) * 4) - 2) - 1);
                  break;
                case GEGl_RIPPLE_WAVE_TYPE_SINE:
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
gegl_chant_class_init (GeglChantClass *klass)
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
