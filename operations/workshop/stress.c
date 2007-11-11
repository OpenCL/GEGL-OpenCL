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

#if GEGL_CHANT_PROPERTIES 

gegl_chant_int (radius,     2, 5000.0, 300, "neighbourhood taken into account")
gegl_chant_int (samples,    0, 1000,   10,    "number of samples to do")
gegl_chant_int (iterations, 0, 1000.0, 10,   "number of iterations (length of exposure)")
gegl_chant_boolean (same_spray, TRUE, "use the same spray for all pixels")
gegl_chant_double (rgamma, 0.0, 8.0, 2.0, "gamma applied to radial distribution")
gegl_chant_double (strength, -10.0, 10.0, 1.0, "amoung of correction 0=none 1.0=full")
gegl_chant_double (gamma, 0.0, 10.0, 1.0, "post correction gamma.")
#else

#define GEGL_CHANT_NAME         stress
#define GEGL_CHANT_SELF         "stress.c"
#define GEGL_CHANT_DESCRIPTION  "Spatio Temporal Retinex-like Envelope with Stochastic Sampling."
#define GEGL_CHANT_CATEGORIES   "enhance"

#define GEGL_CHANT_AREA_FILTER

#define GEGL_CHANT_CLASS_INIT

#include "gegl-chant.h"
#include <math.h>

static void stress (GeglBuffer *src,
                    GeglBuffer *dst,
                    gint        radius,
                    gint        samples,
                    gint        iterations,
                    gboolean    same_spray,
                    gdouble     rgamma,
                    gdouble     strength,
                    gdouble     gamma);

#include <stdlib.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *output;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);

  input = gegl_operation_get_source (operation, context_id, "input");
  output = gegl_operation_get_target (operation, context_id, "output");

  stress (input, output,
          self->radius,
          self->samples,
          self->iterations,
          self->same_spray,
          self->rgamma,
          self->strength,
          self->gamma);

  gegl_buffer_destroy (input);
  return  TRUE;
}

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

  src_buf = g_malloc0 (gegl_buffer_pixel_count (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_pixel_count (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  for (y=radius; y<gegl_buffer_height (dst)+radius; y++)
    {
      gint src_offset = ((gegl_buffer_width (src)*y)+radius)*4;
      for (x=radius; x<gegl_buffer_width (dst)+radius; x++)
        {
          gfloat *center_pix= src_buf + src_offset;
          gfloat  min_envelope[4];
          gfloat  max_envelope[4];

          compute_envelopes (src_buf,
                             gegl_buffer_width (src),
                             gegl_buffer_height (src),
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
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf);
  g_free (src_buf);
  g_free (dst_buf);
}

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantOperation      *filter = GEGL_CHANT_OPERATION (operation);
  area->left = area->right = area->top = area->bottom =
  ceil (filter->radius);
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                     "input");
  if (!in_rect)
    return result;
  return *in_rect;
}

static void class_init (GeglOperationClass *operation_class)
{
  /* we override defined region to avoid growing the size of what is defined
   * by the filter, this also allows the tricks used to treat alpha==0 pixels
   * in the image as source data not to be skipped by the stochastic sampling
   * yielding correct edge behavior.
   */
  operation_class->get_defined_region  = get_defined_region;
}

#endif
