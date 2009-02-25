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
 * Copyright 2007,2009 Øyvind Kolås     <pippin@gimp.org>
 *                     Ivar Farup       <ivarf@hig.no>
 *                     Allesandro Rizzi <rizzi@dti.unimi.it>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (radius, _("Radius"), 2, 3000.0, 300,
                _("Neighbourhood taken into account, this is the radius in pixels taken into account when deciding which colors map to which gray values."))
gegl_chant_int (samples, _("Samples"), 0, 1000, 4,
                _("Number of samples to do per iteration looking for the range of colors."))
gegl_chant_int (iterations, _("Iterations"), 0, 1000, 10,
                _("Number of iterations, a higher number of iterations provides a less noisy results at computational cost."))

gegl_chant_boolean (same_spray, _("Same spray"), FALSE,
                _("Use the same spray for all pixels"))
/*
gegl_chant_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                _("Gamma applied to radial distribution"))
gegl_chant_double (gamma, _("Gamma"), 0.0, 10.0, 1.0,
                _("Post correction gamma."))
*/
#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "c2g.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

#define RGAMMA 2.0
#define GAMMA  1.0

static void stress (GeglBuffer *src,
                    GeglBuffer *dst,
                    gint        radius,
                    gint        samples,
                    gint        iterations,
                    gboolean    same_spray,
                    gdouble     rgamma,
                    gdouble     gamma)
{
  gint x,y;
  gint    dst_offset=0;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 2);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  for (y=radius; y<gegl_buffer_get_height (dst)+radius; y++)
    {
      gint src_offset = ((gegl_buffer_get_width (src)*y)+radius)*4;
      for (x=radius; x<gegl_buffer_get_width (dst)+radius; x++)
        {
          gfloat *pixel= src_buf + src_offset;
          gfloat  min[4];
          gfloat  max[4];

          compute_envelopes (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             x, y,
                             radius, samples,
                             iterations,
                             same_spray,
                             rgamma,
                             min, max);
          { 
            gint   c;
            gfloat  u[3];
            gfloat  v[3];

            for (c=0;c<3;c++)
              u[c]=min[c]-max[c];
            for (c=0;c<3;c++)
              v[c]=min[c]-pixel[c];

            {
              gdouble len = sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);
              if (len <0.01)
                {
                  dst_buf[dst_offset+0] = 0;
                }
              else
                {
                  for (c=0;c<3;c++)
                    u[c] /= len;
                dst_buf[dst_offset+0] = (u[0])*(v[0])+(u[1])*(v[1])+(u[2])*(v[2]);
                }
            }
          }
          dst_buf[dst_offset+1] = src_buf[src_offset+3];

          src_offset+=4;
          dst_offset+=2;
        }
    }
  gegl_buffer_set (dst, NULL, babl_format ("YA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_CHANT_PROPERTIES (operation)->radius);
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
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
          /*o->rgamma*/RGAMMA,
          /*o->gamma*/GAMMA);

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

  operation_class->name        = "gegl:c2g";
  operation_class->categories  = "enhance";
  operation_class->description =
        _("Color to grayscale conversion, uses envelopes formed from spatial "
         " color differences to perform color-feature preserving grayscale "
         " spatial contrast enhancement.");
}

#endif
