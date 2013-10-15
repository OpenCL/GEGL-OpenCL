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
 *           2013 Téo Mazars <teo.mazars@ensimag.fr>
 */

#include "config.h"
#include <math.h>
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int    (wrong_pixels, _("Wrong pixels"),
                   G_MININT, G_MAXINT, 0,
                   _("Number of differing pixels."))

gegl_chant_double (max_diff, _("Maximum difference"),
                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Maximum difference between two pixels."))

gegl_chant_double (avg_diff_wrong, _("Average difference (wrong)"),
                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Average difference between wrong pixels."))

gegl_chant_double (avg_diff_total, _("Average difference (total)"),
                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Average difference between all pixels."))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "image-compare.c"

#include "gegl-chant.h"

#define ERROR_TOLERANCE 0.01
#define SQR(x)          ((x) * (x))

static void
prepare (GeglOperation *self)
{
  gegl_operation_set_format (self, "input",  babl_format ("CIE Lab alpha float"));
  gegl_operation_set_format (self, "aux",    babl_format ("CIE Lab alpha float"));
  gegl_operation_set_format (self, "output", babl_format ("R'G'B' u8"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO         *props        = GEGL_CHANT_PROPERTIES (operation);
  gdouble             max_diff     = 0.0;
  gdouble             diffsum      = 0.0;
  gint                wrong_pixels = 0;
  const Babl         *cielab       = babl_format ("CIE Lab alpha float");
  const Babl         *srgb         = babl_format ("R'G'B' u8");
  GeglBufferIterator *gi;
  gint                index_iter, index_iter2;

  if (aux == NULL)
    return TRUE;

  gi = gegl_buffer_iterator_new (output, result, 0, srgb,
                                 GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  index_iter = gegl_buffer_iterator_add (gi, input, result, 0, cielab,
                                         GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  index_iter2 = gegl_buffer_iterator_add (gi, aux, result, 0, cielab,
                                         GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      guint   k;
      guchar *data_out;
      gfloat *data_in1;
      gfloat *data_in2;

      data_out = (guchar*) gi->data[0];
      data_in1 = (gfloat*) gi->data[index_iter];
      data_in2 = (gfloat*) gi->data[index_iter2];

      for (k = 0; k < gi->length; k++)
        {
          gdouble diff = sqrt (SQR (data_in1[0] - data_in2[0]) +
                               SQR (data_in1[1] - data_in2[1]) +
                               SQR (data_in1[2] - data_in2[2]));

          gdouble alpha_diff = abs (data_in1[3] - data_in2[3]) * 100.0;

          diff = MAX (diff, alpha_diff);

          if (diff >= ERROR_TOLERANCE)
            {
              wrong_pixels++;
              diffsum += diff;

              if (diff > max_diff)
                max_diff = diff;

              data_out[0] = CLAMP ((100 - data_in1[0]) / 100.0 * 64 + 32,
                                   0, 255);
              data_out[1] = CLAMP (diff / max_diff * 255, 0, 255);
              data_out[2] = 0;
            }
          else
            {
              data_out[0] = CLAMP (data_in1[0] / 100.0 * 255, 0, 255);
              data_out[1] = CLAMP (data_in1[0] / 100.0 * 255, 0, 255);
              data_out[2] = CLAMP (data_in1[0] / 100.0 * 255, 0, 255);
            }

          data_out += 3;
          data_in1 += 4;
          data_in2 += 4;
        }
    }

  props->wrong_pixels   = wrong_pixels;
  props->max_diff       = max_diff;
  props->avg_diff_wrong = diffsum / wrong_pixels;
  props->avg_diff_total = diffsum / (result->width * result->height);

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
  operation_class->get_cached_region       = get_cached_region;
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
