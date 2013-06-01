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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (x, _("X"),
                      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 0.0, 1024.0, 1.0,
                      _("X coordinate of the center of the waves"))

gegl_chant_double_ui (y, _("Y"),
                      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 0.0, 1024.0, 1.0,
                      _("Y coordinate of the center of the waves"))

gegl_chant_double    (amplitude, _("Amplitude"),
                      0.0, 1000.0, 25.0,
                      _("Amplitude of the ripple"))

gegl_chant_double    (period, _("Period"),
                      0.0, 1000.0, 200.0,
                      _("Period (wavelength) of the ripple"))

gegl_chant_double    (phi, _("Phase shift"),
                      -1.0, 1.0, 0.0,
                      _("Phase shift"))

gegl_chant_double    (aspect, _("Aspect ratio"),
                      0.1, 10.0, 1.0,
                      _("Aspect ratio"))

gegl_chant_enum      (sampler_type, _("Sampler"),
                      GeglSamplerType, gegl_sampler_type,
                      GEGL_SAMPLER_CUBIC,
                      _("Sampler used internally"))

gegl_chant_boolean   (clamp, _("Clamp deformation"),
                      FALSE,
                      _("Limit deformation in the image area."))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "waves.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);

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
  GeglChantO  *o = GEGL_CHANT_PROPERTIES (operation);
  gint         x = result->x; /* initial x                   */
  gint         y = result->y; /*           and y coordinates */

  gfloat      *dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof (gfloat));

  gfloat      *out_pixel = dst_buf;

  GeglSampler *sampler = gegl_buffer_sampler_new (input,
                                                  babl_format ("RGBA float"),
                                                  o->sampler_type);

  gint         n_pixels = result->width * result->height;
  GeglAbyssPolicy abyss = o->clamp ? GEGL_ABYSS_CLAMP : GEGL_ABYSS_NONE;

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

  while (n_pixels--)
    {
      gdouble radius;
      gdouble shift;
      gdouble dx;
      gdouble dy;
      gdouble ux;
      gdouble uy;

      dx = (x - o->x) * scalex;
      dy = (y - o->y) * scaley;

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

      /* update x and y coordinates */
      x++;
      if (x >= result->x + result->width)
        {
          x = result->x;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_slice_free1 (result->width * result->height * 4 * sizeof (gfloat), dst_buf);

  g_object_unref (sampler);

  return TRUE;
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
    "name",        "gegl:waves",
    "categories",  "distort",
    "description", _("Distort the image with waves"),
    NULL);
}

#endif
