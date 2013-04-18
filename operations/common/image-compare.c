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
 * Copyright 2010 Øyvind Kolås <pippin@gimp.org>
 *           2012 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"
#include <math.h>
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (wrong_pixels, _("Wrong pixels"), G_MININT, G_MAXINT, 0, _("Number of differing pixels."))
gegl_chant_double (max_diff, _("Maximum difference"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, _("Maximum difference between two pixels."))
gegl_chant_double (avg_diff_wrong, _("Average difference (wrong)"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, _("Average difference between wrong pixels."))
gegl_chant_double (avg_diff_total, _("Average difference (total)"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, _("Average difference between all pixels."))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "image-compare.c"

#include "gegl-chant.h"


static void
prepare (GeglOperation *self)
{
  gegl_operation_set_format (self, "input", babl_format ("CIE Lab float"));
  gegl_operation_set_format (self, "aux", babl_format ("CIE Lab float"));
  gegl_operation_set_format (self, "output", babl_format ("R'G'B' u8"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  return result;
}

#define SQR(x) ((x) * (x))

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO  *props        = GEGL_CHANT_PROPERTIES (operation);
  gdouble      max_diff     = 0.0;
  gdouble      diffsum      = 0.0;
  gint         wrong_pixels = 0;
  const Babl*  cielab       = babl_format ("CIE Lab float");
  const Babl*  rgbaf        = babl_format ("RGBA float");
  const Babl*  srgb         = babl_format ("R'G'B' u8");
  gint         pixels, i;
  gfloat      *in_buf, *aux_buf, *a, *b;
  gfloat      *in_buf_rgba, *aux_buf_rgba, *aalpha, *balpha;
  guchar      *out_buf, *out;

  if (aux == NULL)
    return TRUE;

  in_buf = g_malloc (result->height * result->width * babl_format_get_bytes_per_pixel (cielab));
  aux_buf = g_malloc (result->height * result->width * babl_format_get_bytes_per_pixel (cielab));

  in_buf_rgba = g_malloc (result->height * result->width * babl_format_get_bytes_per_pixel (rgbaf));
  aux_buf_rgba = g_malloc (result->height * result->width * babl_format_get_bytes_per_pixel (rgbaf));

  out_buf = g_malloc (result->height * result->width * babl_format_get_bytes_per_pixel (srgb));

  gegl_buffer_get (input, result, 1.0, cielab, in_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (aux, result, 1.0, cielab, aux_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (input, result, 1.0, rgbaf, in_buf_rgba, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (aux, result, 1.0, rgbaf, aux_buf_rgba, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  a   = in_buf;
  b   = aux_buf;
  out = out_buf;

  aalpha = in_buf_rgba;
  balpha = aux_buf_rgba;

  pixels = result->width * result->height;

  for (i = 0; i < pixels; i++)
    {
      gdouble diff = sqrt (SQR(a[0] - b[0])+
                           SQR(a[1] - b[1])+
                           SQR(a[2] - b[2]));

      gdouble alpha_diff = abs(aalpha[3] - balpha[3]) * 100.0;

      if (alpha_diff > diff)
        diff = alpha_diff;

      if (diff >= 0.01)
        {
          wrong_pixels++;
          diffsum += diff;
          if (diff > max_diff)
            max_diff = diff;
          out[0] = (diff / 100.0 * 255);
          out[1] = 0;
          out[2] = a[0] / 100.0 * 255;
        }
      else
        {
          out[0] = a[0] / 100.0 * 255;
          out[1] = a[0] / 100.0 * 255;
          out[2] = a[0] / 100.0 * 255;
        }
      a   += 3;
      aalpha += 4;
      b   += 3;
      balpha += 4;
      out += 3;
    }

  a   = in_buf;
  b   = aux_buf;
  out = out_buf;

  aalpha = in_buf_rgba;
  balpha = aux_buf_rgba;

  if (wrong_pixels)
    for (i = 0; i < pixels; i++)
      {
        gdouble diff = sqrt (SQR(a[0] - b[0])+
                             SQR(a[1] - b[1])+
                             SQR(a[2] - b[2]));

        gdouble alpha_diff = abs(aalpha[3] - balpha[3]) * 100.0;

        if (alpha_diff > diff)
          diff = alpha_diff;

        if (diff >= 0.01)
          {
            out[0] = (100 - a[0]) / 100.0 * 64 + 32;
            out[1] = (diff / max_diff * 255);
            out[2] = 0;
          }
        else
          {
            out[0] = a[0] / 100.0 * 255;
            out[1] = a[0] / 100.0 * 255;
            out[2] = a[0] / 100.0 * 255;
          }
        a   += 3;
        aalpha += 4;
        b   += 3;
        balpha += 4;
        out += 3;
      }

  gegl_buffer_set (output, result, 1.0, srgb, out_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (in_buf);
  g_free (aux_buf);
  g_free (out_buf);

  g_free (in_buf_rgba);
  g_free (aux_buf_rgba);

  props->wrong_pixels   = wrong_pixels;
  props->max_diff       = max_diff;
  props->avg_diff_wrong = diffsum / wrong_pixels;
  props->avg_diff_total = diffsum / pixels;

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  composer_class->process                  = process;

  gegl_operation_class_set_keys (operation_class,
                                 "name"       , "gegl:image-compare",
                                 "categories" , "programming",
                                 "description", _("Compares if input and aux buffers are "
                                                  "different. Results are saved in the "
                                                  "properties."),
                                 NULL);
}

#endif
