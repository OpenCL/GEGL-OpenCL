/* Minimum envelope, as used by STRESS.
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
 * Copyright 2007 Øyvind Kolås     <pippin@gimp.org>
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (radius, "Radius", 2, 5000.0, 50,
                "Neighbourhood taken into account")
gegl_chant_int (samples, "Samples", 0, 1000, 3,
                "Number of samples to do")
gegl_chant_int (iterations, "Iterations", 0, 1000.0, 20,
                "Number of iterations (length of exposure)")
gegl_chant_boolean (same_spray, "Same spray", FALSE, "")
gegl_chant_double (rgamma, "Radial gamma", 0.0, 8.0, 1.8,
                   "gamma applied to radial distribution")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "min-envelope.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

static void min_envelope (GeglBuffer *src,
                          GeglBuffer *dst,
                          gint        radius,
                          gint        samples,
                          gint        iterations,
                          gboolean    same_spray,
                          gdouble     rgamma)
{
  gint    x, y;
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
          gfloat *center_pix= src_buf + offset;
          gfloat  min_envelope[4];

          compute_envelopes (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             x, y,
                             radius, samples,
                             iterations,
                             same_spray,
                             rgamma,
                             min_envelope, NULL);

         {
          gint c;
          for (c=0; c<3;c++)
            dst_buf[offset+c] = min_envelope[c];
          dst_buf[offset+c] = center_pix[c];
         }
          offset+=4;
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
  GeglRectangle compute  = gegl_operation_get_required_for_output (operation, "input", result);

  if (o->radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);

      min_envelope (temp_in, output, o->radius, o->samples, o->iterations, o->same_spray, o->rgamma);
      g_object_unref (temp_in);
    }

  return  TRUE;
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

  operation_class->name        = "min-envelope";
  operation_class->categories  = "enhance";
  operation_class->description = "Minimum envelope as used by STRESS.";
}
#endif
