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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås <oeyvindk@hig.no>
 *                Ivar Farup   <ivarf@hig.no>
 */

#if GEGL_CHANT_PROPERTIES 

gegl_chant_int (radius,     2, 5000.0, 384, "neighbourhood taken into account")
gegl_chant_int (samples,    0, 1000,    3,  "number of samples to do")
gegl_chant_int (iterations, 0, 1000.0, 23, "number of iterations (length of exposure)")
gegl_chant_boolean (same_spray, FALSE, "")
gegl_chant_double (rgamma, 0.0, 8.0, 1.8, "gamma applied to radial distribution")
gegl_chant_double (strength,  -8, 8,  0.5, "how much the local optimum separation should be taken into account.")
gegl_chant_double (gamma, 0.0, 10.0, 1.6, "post correction gamma.")
#else

#define GEGL_CHANT_NAME         c2g
#define GEGL_CHANT_SELF         "c2g.c"
#define GEGL_CHANT_DESCRIPTION  "Color to grayscale conversion that uses, spatial color differences to perform local grayscale contrast enhancement."
#define GEGL_CHANT_CATEGORIES   "enhance"

#define GEGL_CHANT_AREA_FILTER

#include "gegl-chant.h"
#include <math.h>
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


  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  {
    GeglRectangle   *result = gegl_operation_result_rect (operation, context_id);
    GeglBuffer      *temp_in;
    GeglRectangle    compute  = gegl_operation_compute_input_request (operation, "inputt", gegl_operation_need_rect (operation, context_id));

    temp_in = gegl_buffer_create_sub_buffer (input, &compute);
    output = gegl_buffer_new (&compute, babl_format ("RGBA float"));

    c2g (temp_in, output, self->radius, self->samples, self->iterations, self->same_spray, self->rgamma, self->strength, self->gamma);
    g_object_unref (temp_in);
    

    {
      GeglBuffer *cropped = gegl_buffer_create_sub_buffer (output, result);

      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (cropped));
      g_object_unref (output);
    }
  }
  return  TRUE;
}

#include "envelopes.h"

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


  src_buf = g_malloc0 (gegl_buffer_pixel_count (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_pixel_count (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);
  for (y=radius; y<gegl_buffer_height (dst)-radius; y++)
    {
      gint offset = ((gegl_buffer_width (src)*y)+radius)*4;
      for (x=radius; x<gegl_buffer_width (dst)-radius; x++)
        {
          gfloat  min_envelope[4];
          gfloat  max_envelope[4];
          gfloat *pixel = src_buf + offset;

          compute_envelopes (src_buf,
                             gegl_buffer_width (src),
                             gegl_buffer_height (src),
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

#endif
