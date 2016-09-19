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
 * Copyright 2010 Danny Robson      <danny@blubinc.net>
 * (pfstmo)  2007 Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (brightness, _("Brightness"), 0.0)
    description(_("Overall brightness of the image"))
    value_range (-100.0, 100.0)

property_double (chromatic, _("Chromatic adaptation"), 0.0)
    description(_("Adaptation to color variation across the image"))
    value_range (0.0, 1.0)

property_double (light, _("Light adaptation"), 1.0)
    description(_("Adaptation to light variation across the image"))
    value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     reinhard05
#define GEGL_OP_C_SOURCE reinhard05.c

#include "gegl-op.h"


typedef struct {
  gfloat min, max, avg, range;
  guint  num;
} stats;


static const gchar *OUTPUT_FORMAT = "RGBA float";


static void
reinhard05_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format (OUTPUT_FORMAT));
  gegl_operation_set_format (operation, "output", babl_format (OUTPUT_FORMAT));
}

static GeglRectangle
reinhard05_get_required_for_output (GeglOperation       *operation,
                                    const gchar         *input_pad,
                                    const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}


static GeglRectangle
reinhard05_get_cached_region (GeglOperation       *operation,
                              const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static void
reinhard05_stats_start (stats *s)
{
  g_return_if_fail (s);

  s->min   = G_MAXFLOAT;
  s->max   = G_MINFLOAT;
  s->avg   = 0.0;
  s->range = NAN;
  s->num   = 0;
};


static void
reinhard05_stats_update (stats *s,
                         gfloat value)
{
  g_return_if_fail (s);
  g_return_if_fail (!isinf (value));
  g_return_if_fail (!isnan (value));

  s->min  = MIN (s->min, value);
  s->max  = MAX (s->max, value);
  s->avg += value;
  s->num += 1;
}


static void
reinhard05_stats_finish (stats *s)
{
  g_return_if_fail (s->num !=    0.0);
  g_return_if_fail (s->max >= s->min);

  s->avg   /= s->num;
  s->range  = s->max - s->min;
}


static gboolean
reinhard05_process (GeglOperation       *operation,
                    GeglBuffer          *input,
                    GeglBuffer          *output,
                    const GeglRectangle *result,
                    gint                 level)
{
  const GeglProperties *o = GEGL_PROPERTIES (operation);

  const gint  pix_stride = 4, /* RGBA */
              RGB        = 3;

  gfloat *lum,
         *pix;
  gfloat  key, contrast, intensity,
          chrom      =       o->chromatic,
          chrom_comp = 1.0 - o->chromatic,
          light      =       o->light,
          light_comp = 1.0 - o->light;

  stats   world_lin,
          world_log,
          channel [RGB],
          normalise;

  gint    i, c;

  g_return_val_if_fail (operation, FALSE);
  g_return_val_if_fail (input, FALSE);
  g_return_val_if_fail (output, FALSE);
  g_return_val_if_fail (result, FALSE);

  g_return_val_if_fail (babl_format_get_n_components (babl_format (OUTPUT_FORMAT)) == pix_stride, FALSE);

  g_return_val_if_fail (chrom      >= 0.0 && chrom      <= 1.0, FALSE);
  g_return_val_if_fail (chrom_comp >= 0.0 && chrom_comp <= 1.0, FALSE);
  g_return_val_if_fail (light      >= 0.0 && light      <= 1.0, FALSE);
  g_return_val_if_fail (light_comp >= 0.0 && light_comp <= 1.0, FALSE);


  /* Obtain the pixel data */
  lum = g_new (gfloat, result->width * result->height),
  gegl_buffer_get (input, result, 1.0, babl_format ("Y float"),
                   lum, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  pix = g_new (gfloat, result->width * result->height * pix_stride);
  gegl_buffer_get (input, result, 1.0, babl_format (OUTPUT_FORMAT),
                   pix, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /* Collect the image stats, averages, etc */
  reinhard05_stats_start (&world_lin);
  reinhard05_stats_start (&world_log);
  reinhard05_stats_start (&normalise);
  for (i = 0; i < RGB; ++i)
    {
      reinhard05_stats_start (channel + i);
    }

  for (i = 0; i < result->width * result->height; ++i)
    {
      reinhard05_stats_update (&world_lin,                 lum[i] );
      reinhard05_stats_update (&world_log, logf (2.3e-5f + lum[i]));

      for (c = 0; c < RGB; ++c)
        {
          reinhard05_stats_update (channel + c, pix[i * pix_stride + c]);
        }
    }

  g_return_val_if_fail (world_lin.min >= 0.0, FALSE);

  reinhard05_stats_finish (&world_lin);
  reinhard05_stats_finish (&world_log);
  for (i = 0; i < RGB; ++i)
    {
      reinhard05_stats_finish (channel + i);
    }

  /* Calculate key parameters */
  key       = (logf (world_lin.max) -                 world_log.avg) /
              (logf (world_lin.max) - logf (2.3e-5f + world_lin.min));
  contrast  = 0.3 + 0.7 * powf (key, 1.4);
  intensity = expf (-o->brightness);

  g_return_val_if_fail (contrast >= 0.3 && contrast <= 1.0, FALSE);

  /* Apply the operator */
  for (i = 0; i < result->width * result->height; ++i)
    {
      gfloat local, global, adapt;

      if (lum[i] == 0.0)
        continue;

      for (c = 0; c < RGB; ++c)
        {
          gfloat *_p = pix + i * pix_stride + c,
                   p = *_p;

          local  = chrom      * p +
                   chrom_comp * lum[i];
          global = chrom      * channel[c].avg +
                   chrom_comp * world_lin.avg;
          adapt  = light      * local +
                   light_comp * global;

          p  /= p + powf (intensity * adapt, contrast);
          *_p = p;
          reinhard05_stats_update (&normalise, p);
        }
    }

  /* Normalise the pixel values */
  reinhard05_stats_finish (&normalise);

  for (i = 0; i < result->width * result->height; ++i)
    {
      for (c = 0; c < pix_stride; ++c)
        {
          gfloat *p = pix + i * pix_stride + c;
          *p        = (*p - normalise.min) / normalise.range;
        }
    }

  /* Cleanup and set the output */
  gegl_buffer_set (output, result, 0, babl_format (OUTPUT_FORMAT), pix,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (pix);
  g_free (lum);

  return TRUE;
}


/*
 */
static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = reinhard05_process;

  operation_class->prepare                 = reinhard05_prepare;
  operation_class->get_required_for_output = reinhard05_get_required_for_output;
  operation_class->get_cached_region       = reinhard05_get_cached_region;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
  "name",      "gegl:reinhard05",
  "title",      _("Reinhard 2005 Tone Mapping"),
  "categories" , "tonemapping",
  "description",
        _("Adapt an image, which may have a high dynamic range, for "
          "presentation using a low dynamic range. This is an efficient "
          "global operator derived from simple physiological observations, "
          "producing luminance within the range 0.0-1.0"),
        NULL);
}

#endif

