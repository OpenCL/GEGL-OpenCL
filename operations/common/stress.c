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


#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 300)
    description(_("Neighborhood taken into account, for enhancement ideal values are close to the longest side of the image, increasing this increases the runtime"))
    value_range (2, 6000)
    ui_range    (2, 1000)
    ui_gamma    (1.6)
    ui_meta     ("unit", "pixel-distance")

property_int (samples, _("Samples"), 5)
    description(_("Number of samples to do per iteration looking for the range of colors"))
    value_range (2, 500)
    ui_range    (3, 17)

property_int (iterations, _("Iterations"), 5)
    description(_("Number of iterations, a higher number of iterations provides a less noisy rendering at a computational cost"))
    value_range (1, 1000)
    ui_range    (1, 30)

/*

property_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                _("Gamma applied to radial distribution"))

*/



#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     stress
#define GEGL_OP_C_SOURCE stress.c

#include "gegl-op.h"

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
                    gdouble              rgamma,
                    gint                 level)
{
  const Babl *format = babl_format ("RGBA float");

  if (dst_rect->width > 0 && dst_rect->height > 0)
  {
    GeglBufferIterator *i = gegl_buffer_iterator_new (dst, dst_rect, 0, babl_format("RaGaBaA float"),
                                                      GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
    GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src, format, GEGL_SAMPLER_NEAREST, level);

    while (gegl_buffer_iterator_next (i))
    {
      gint x,y;
      gint    dst_offset=0;
      gfloat *dst_buf = i->data[0];

      for (y=i->roi[0].y; y < i->roi[0].y + i->roi[0].height; y++)
        {
          for (x=i->roi[0].x; x < i->roi[0].x + i->roi[0].width; x++)
            {
              gfloat  min[4];
              gfloat  max[4];
              gfloat  pixel[4];

              compute_envelopes (src, sampler,
                                 x, y,
                                 radius, samples,
                                 iterations,
                                 FALSE, /* same spray */
                                 rgamma,
                                 min, max, pixel, format);
              {
                /* this should be replaced with a better/faster projection of
                 * pixel onto the vector spanned by min -> max, currently
                 * computed by comparing the distance to min with the sum
                 * of the distance to min/max.
                 */

              gint c;
              for (c=0;c<3;c++)
                {
                  gfloat delta = max[c]-min[c];
                  if (delta != 0)
                    {
                      dst_buf[dst_offset+c] =
                         (pixel[c]-min[c])/delta;
                    }
                  else
                    {
                      dst_buf[dst_offset+c] = 0.5;
                    }
                }

                dst_buf[dst_offset+3] = pixel[3];
                dst_offset+=4;
              }
            }
          }
    }
    g_object_unref (sampler);
  }
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_PROPERTIES (operation)->radius);

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
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle compute;
  compute = gegl_operation_get_required_for_output (operation, "input",result);

  stress (input, &compute, output, result,
          o->radius,
          o->samples,
          o->iterations,
          RGAMMA /*o->rgamma,*/, level);

  return  TRUE;
}

static const gchar *composition =
    "<?xml version='1.0'             encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:stress'>"
    "  <params>"
    "    <param name='radius'>200</param>"
    "    <param name='iterations'>30</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

static void
gegl_op_class_init (GeglOpClass *klass)
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
    "name",                  "gegl:stress",
    "title",                 _("Spatio Temporal Retinex-like Envelope with Stochastic Sampling"),
    "categories",            "enhance:tonemapping",
    "reference-composition", composition,
    "description",
        _("Spatio Temporal Retinex-like Envelope with Stochastic Sampling"),
        NULL);
}

#endif
