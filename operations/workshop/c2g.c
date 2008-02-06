/* A spatial color to greyscale converter.
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
 * Copyright 2007 Øyvind Kolås <oeyvindk@hig.no>
 *                Ivar Farup   <ivarf@hig.no>
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (radius, "Radius", 2, 5000.0, 384,
                "Neighbourhood taken into account")
gegl_chant_int (samples, "Samples", 0, 1000,    3,
                "Number of samples to do")
gegl_chant_int (iterations, "Iteration", 0, 1000.0, 23,
                "Number of iterations (length of exposure)")
gegl_chant_boolean (same_spray, "Same spray", FALSE, "")
gegl_chant_double (rgamma,  "Radial gamma", 0.0, 8.0, 1.8,
                   "Gamma applied to radial distribution")
gegl_chant_double (strength, "Strength", -8, 8,  0.5,
                   "How much the local optimum separation should be taken into account.")
gegl_chant_double (gamma, "Gamma", 0.0, 10.0, 1.6, "post correction gamma.")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "c2g.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

#define sq(a) ((a)*(a))

static void c2g (GeglBuffer *src,
                 GeglBuffer *dst,
                 gint        radius,
                 gint        samples,
                 gint        iterations,
                 gboolean    same_spray,
                 gdouble     rgamma,
                 gfloat      strength,
                 gfloat      gamma);

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);
  area->left = area->right = area->top = area->bottom = ceil (o->radius);

  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *temp_in;
  GeglRectangle compute = gegl_operation_compute_input_request (operation, "input", result);

  temp_in = gegl_buffer_create_sub_buffer (input, &compute);

  c2g (temp_in, output, o->radius, o->samples, o->iterations, o->same_spray,
       o->rgamma, o->strength, o->gamma);
  g_object_unref (temp_in);

  return  TRUE;
}

static void c2g (GeglBuffer *src,
                 GeglBuffer *dst,
                 gint        radius,
                 gint        samples,
                 gint        iterations,
                 gboolean    same_spray,
                 gdouble     rgamma,
                 gfloat      strength,
                 gfloat      gamma)
{
  gint x,y;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);
  for (y=radius; y<gegl_buffer_get_height (dst)-radius; y++)
    {
      gint offset = ((gegl_buffer_get_width (src)*y)+radius)*4;
      for (x=radius; x<gegl_buffer_get_width (dst)-radius; x++)
        {
          gfloat  min_envelope[4];
          gfloat  max_envelope[4];
          gfloat *pixel = src_buf + offset;

          compute_envelopes (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             x, y,
                             radius, samples,
                             iterations,
                             same_spray,
                             rgamma,
                             min_envelope, max_envelope);

          { /* now having a local blackpoint and a local white point
               we just wonder whether we are more black than white */
            gfloat gray;
            gint   c;

            gray = pixel[0]*0.212671 + pixel[1] * 0.715160 + pixel[2] * 0.072169;
            {
              gfloat nominator = 0;
              gfloat denominator = 0;
              for (c=0; c<3; c++)
                {
                  gfloat delta = max_envelope[c]-min_envelope[c];
                  nominator += (pixel[c] - min_envelope[c]) * delta;
                  denominator += delta*delta;
                }

              if (denominator>0.000) /* if we found a range, modify the result */
                {
                  gray *= (1.0-strength);

                  if (gamma==1.0)
                    {
                     gray += strength * (nominator/denominator);
                    }
                  else
                    {
                     gray += pow (strength * (nominator/denominator), gamma);
                    }

                }
            }

            for (c=0; c<3;c++)
              dst_buf[offset+c] = gray;
            dst_buf[offset+c] = pixel[c]; /* alpha is unmodified */
          }
          offset+=4;
        }
    }
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "c2g";
  operation_class->categories  = "enhance";
  operation_class->description =
        "Color to grayscale conversion that uses, spatial color differences to perform local grayscale contrast enhancement.";
}

#endif
