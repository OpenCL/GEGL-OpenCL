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

gegl_chant_int_ui (radius, _("Radius"), 2, 5000.0, 300, 2, 2000, 1.0,
                _("Neighborhood taken into account, for enhancement ideal values are close to the longest side of the image, increasing this increases the runtime"))
gegl_chant_int_ui (samples, _("Samples"), 2, 200, 5, 2, 10, 1.0,
                _("Number of samples to do per iteration looking for the range of colors"))
gegl_chant_int_ui (iterations, _("Iterations"), 1, 200, 5, 1, 10, 1.0,
                _("Number of iterations, a higher number of iterations provides a less noisy rendering at a computational cost"))


/*

gegl_chant_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                _("Gamma applied to radial distribution"))

*/



#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "stress.c"

#include "gegl-chant.h"

#define RGAMMA   2.0
#define GAMMA    1.0

#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

static void stress (GeglBuffer          *src,
                    const GeglRectangle *src_rect,
                    GeglBuffer          *dst,
                    const GeglRectangle *dst_rect,
                    gint                 radius,
                    gint                 samples,
                    gint                 iterations,
                    gdouble              rgamma)
{
  gint x,y;
  gint    dst_offset=0;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint    inw = src_rect->width;
  gint    inh = src_rect->height;
  gint   outw = dst_rect->width;

  /* this use of huge linear buffers should be avoided and
   * most probably would lead to great speed ups
   */

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y=radius; y<dst_rect->height+radius; y++)
    {
      gint src_offset = (inw*y+radius)*4;
      for (x=radius; x<outw+radius; x++)
        {
          gfloat *center_pix= src_buf + src_offset;
          gfloat  min_envelope[4];
          gfloat  max_envelope[4];

          compute_envelopes (src_buf,
                             inw, inh,
                             x, y,
                             radius, samples,
                             iterations,
                             FALSE, /* same-spray */
                             rgamma,
                             min_envelope, max_envelope);
           {
              gint c;
              for (c=0;c<3;c++)
                {
                  gfloat delta = max_envelope[c]-min_envelope[c];
                  if (delta != 0)
                    {
                      dst_buf[dst_offset+c] =
                         (center_pix[c]-min_envelope[c])/delta;
                    }
                  else
                    {
                      dst_buf[dst_offset+c] = 0.5;
                    }
                }
           }
          dst_buf[dst_offset+3] = src_buf[src_offset+3];
          src_offset+=4;
          dst_offset+=4;
        }
    }
  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
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
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;
  compute = gegl_operation_get_required_for_output (operation, "input",result);

  stress (input, &compute, output, result,
          o->radius,
          o->samples,
          o->iterations,
          RGAMMA /*o->rgamma,*/);

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
  /* we override get_bounding_box to avoid growing the size of what is defined
   * by the filter. This also allows the tricks used to treat alpha==0 pixels
   * in the image as source data not to be skipped by the stochastic sampling
   * yielding correct edge behavior.
   */
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:stress",
    "categories" , "enhance",
    "description",
        _("Spatio Temporal Retinex-like Envelope with Stochastic Sampling"),
        NULL);
}

#endif
