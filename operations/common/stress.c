/* STRESS, Spatio Temporal Retinex Envelope with Stochastic Sampling
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
 * Copyright 2007 Øyvind Kolås     <oeyvindk@hig.no>
 *                Ivar Farup       <ivarf@hig.no>
 *                Allesandro Rizzi <rizzi@dti.unimi.it>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (radius, _("Radius"), 2, 5000.0, 300,
                _("Neighbourhood taken into account"))
gegl_chant_int (samples, _("Samples"), 0, 1000, 10,
                _("Number of samples to do"))
gegl_chant_int (iterations, _("Iterations"), 0, 1000, 10,
                _("Number of iterations (length of exposure)"))
gegl_chant_boolean (same_spray, _("Same spray"), TRUE,
                    _("Use the same spray for all pixels"))
gegl_chant_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                   _("Gamma applied to radial distribution"))
gegl_chant_double (strength, _("Strength"), -10.0, 10.0, 1.0,
                   _("Amount of correction 0=none 1.0=full"))
gegl_chant_double (gamma, _("Gamma"), 0.0, 10.0, 1.0,
                   _("Post correction gamma"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "stress.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

static void stress (GeglBuffer *src,
                    GeglBuffer *dst,
                    gint        radius,
                    gint        samples,
                    gint        iterations,
                    gboolean    same_spray,
                    gdouble     rgamma,
                    gdouble     strength,
                    gdouble     gamma)
{
  gint x,y;
  gint    dst_offset=0;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  for (y=radius; y<gegl_buffer_get_height (dst)+radius; y++)
    {
      gint src_offset = ((gegl_buffer_get_width (src)*y)+radius)*4;
      for (x=radius; x<gegl_buffer_get_width (dst)+radius; x++)
        {
          gfloat *center_pix= src_buf + src_offset;
          gfloat  min_envelope[4];
          gfloat  max_envelope[4];

          compute_envelopes (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             x, y,
                             radius, samples,
                             iterations,
                             same_spray,
                             rgamma,
                             min_envelope, max_envelope);
         {
          gint c;
          gfloat pixel[3];
          for (c=0;c<3;c++)
            {
              pixel[c] = center_pix[c];
              if (min_envelope[c]!=max_envelope[c])
                {
                  gfloat scaled = (pixel[c]-min_envelope[c])/(max_envelope[c]-min_envelope[c]);
                  pixel[c] *= (1.0-strength);
                  pixel[c] = strength * scaled;
                }
            }
          if (gamma==1.0)
            {
              for (c=0; c<3;c++)
                dst_buf[dst_offset+c] = pixel[c];
            }
          else
            {
              for (c=0; c<3;c++)
                dst_buf[dst_offset+c] = pow(pixel[c],gamma);
            }
          dst_buf[dst_offset+c] = center_pix[c];
         }
          src_offset+=4;
          dst_offset+=4;
        }
    }
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_CHANT_PROPERTIES (operation)->radius);

  gegl_operation_set_format (operation, "output",
                             babl_format ("RaGaBaA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");
  if (!in_rect)
    return result;
  return *in_rect;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  stress (input, output,
          o->radius,
          o->samples,
          o->iterations,
          o->same_spray,
          o->rgamma,
          o->strength,
          o->gamma);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare  = prepare;
  /* we override defined region to avoid growing the size of what is defined
   * by the filter. This also allows the tricks used to treat alpha==0 pixels
   * in the image as source data not to be skipped by the stochastic sampling
   * yielding correct edge behavior.
   */
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->name        = "stress";
  operation_class->categories  = "enhance";
  operation_class->description =
        _("Spatio Temporal Retinex-like Envelope with Stochastic Sampling.");
}

#endif
