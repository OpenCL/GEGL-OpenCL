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
 * Copyright 2008 Jan Heller <jan.heller (at) matfyz.cz>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define LOWEST_TEMPERATURE     1000
#define HIGHEST_TEMPERATURE   12000

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (original_temperature, "Original temperature", LOWEST_TEMPERATURE, HIGHEST_TEMPERATURE, 6500, "Estimated temperature of the light source in Kelvin the image was taken with.")
gegl_chant_double (intended_temperature, "Intended temperature", LOWEST_TEMPERATURE, HIGHEST_TEMPERATURE, 6500, "Corrected estimation of the temperature of the light source in Kelvin.")

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "color-temperature.c"

#include "gegl-chant.h"

static const gfloat rgb_r55[3][12];

static void
convert_k_to_rgb (gfloat  temperature,
                  gfloat *rgb)
{
  gint channel;

  if (temperature < LOWEST_TEMPERATURE)
    temperature = LOWEST_TEMPERATURE;

  if (temperature > HIGHEST_TEMPERATURE)
    temperature = HIGHEST_TEMPERATURE;

  /* Evaluation of an approximation of the Planckian locus in linear RGB space
   * by rational functions of degree 5 using Horner's scheme
   * f(x) =  (p1*x^5 + p2*x^4 + p3*x^3 + p4*x^2 + p5*x + p6) /
   *            (x^5 + q1*x^4 + q2*x^3 + q3*x^2 + q4*x + q5)
   */
  for (channel = 0; channel < 3; channel++)
    {
      gfloat nomin, denom;
      gint   deg;

      nomin = rgb_r55[channel][0];
      for (deg = 1; deg < 6; deg++)
        nomin = nomin * temperature + rgb_r55[channel][deg];

      denom = rgb_r55[channel][6];
      for (deg = 1; deg < 6; deg++)
        denom = denom * temperature + rgb_r55[channel][6 + deg];

      rgb[channel] = nomin / denom;
    }
}


static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
notify (GObject    *object,
        GParamSpec *pspec)
{
  if (strcmp (pspec->name, "original-temperature") == 0 ||
      strcmp (pspec->name, "intended-temperature") == 0)
    {
      GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

      /* one of the properties has changed,
       * invalidate the preprocessed coefficients
       */
      if (o->chant_data)
        {
          g_free (o->chant_data);
          o->chant_data = NULL;
        }
    }

  if (G_OBJECT_CLASS (gegl_chant_parent_class)->notify)
    G_OBJECT_CLASS (gegl_chant_parent_class)->notify (object, pspec);
}


static gfloat *
preprocess (GeglChantO *o)
{
  gfloat *coeffs = g_new (gfloat, 3);
  gfloat  original_temperature_rgb[3];
  gfloat  intended_temperature_rgb[3];

  convert_k_to_rgb (o->original_temperature, original_temperature_rgb);
  convert_k_to_rgb (o->intended_temperature, intended_temperature_rgb);

  coeffs[0] = original_temperature_rgb[0] / intended_temperature_rgb[0];
  coeffs[1] = original_temperature_rgb[1] / intended_temperature_rgb[1];
  coeffs[2] = original_temperature_rgb[2] / intended_temperature_rgb[2];

  return coeffs;
}

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO   *o         = GEGL_CHANT_PROPERTIES (op);
  gfloat       *in_pixel  = in_buf;
  gfloat       *out_pixel = out_buf;
  const gfloat *coeffs    = o->chant_data;

  in_pixel = in_buf;
  out_pixel = out_buf;

  if (! coeffs)
    {
      coeffs = o->chant_data = preprocess (o);
    }

  while (n_pixels--)
    {
      out_pixel[0] = in_pixel[0] * coeffs[0];
      out_pixel[1] = in_pixel[1] * coeffs[1];
      out_pixel[2] = in_pixel[2] * coeffs[2];
      out_pixel[3] = in_pixel[3];

      in_pixel += 4;
      out_pixel += 4;
    }

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  object_class       = G_OBJECT_CLASS (klass);
  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  object_class->notify   = notify;

  operation_class->prepare = prepare;

  point_filter_class->process = process;

  operation_class->name        = "gegl:color-temperature";
  operation_class->categories  = "color";
  operation_class->description =
        _("Allows changing the color temperature of an image.");
}

/* Coefficients of rational functions of degree 5 fitted per color channel to
 * the linear RGB coordinates of the range 1000K-12000K of the Planckian locus
 * with the 20K step. Original CIE-xy data from
 *
 * http://www.aim-dtp.net/aim/technology/cie_xyz/k2xy.txt
 *
 * converted to the linear RGB space assuming the ITU-R BT.709-5/sRGB primaries
 */
static const gfloat rgb_r55[3][12] =
{
  {
     6.9389923563552169e-01,  2.7719388100974670e+03,
     2.0999316761104289e+07, -4.8889434162208414e+09,
    -1.1899785506796783e+07, -4.7418427686099203e+04,
     1.0000000000000000e+00,  3.5434394338546258e+03,
    -5.6159353379127791e+05,  2.7369467137870544e+08,
     1.6295814912940913e+08,  4.3975072422421846e+05
  },
  {
     9.5417426141210926e-01,  2.2041043287098860e+03,
    -3.0142332673634286e+06, -3.5111986367681120e+03,
    -5.7030969525354260e+00,  6.1810926909962016e-01,
     1.0000000000000000e+00,  1.3728609973644000e+03,
     1.3099184987576159e+06, -2.1757404458816318e+03,
    -2.3892456292510311e+00,  8.1079012401293249e-01
  },
  {
    -7.1151622540856201e+10,  3.3728185802339764e+16,
    -7.9396187338868539e+19,  2.9699115135330123e+22,
    -9.7520399221734228e+22, -2.9250107732225114e+20,
     1.0000000000000000e+00,  1.3888666482167408e+16,
     2.3899765140914549e+19,  1.4583606312383295e+23,
     1.9766018324502894e+22,  2.9395068478016189e+18
  }
};

#endif
