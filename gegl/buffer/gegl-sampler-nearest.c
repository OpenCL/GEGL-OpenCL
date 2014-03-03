/* This file is part of GEGL
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
 */

#include "config.h"
#include <math.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-sampler-nearest.h"

enum
{
  PROP_0,
  PROP_LAST
};

static void
gegl_sampler_nearest_get (      GeglSampler*    restrict  self,
                          const gdouble                   absolute_x,
                          const gdouble                   absolute_y,
                                GeglMatrix2              *scale,
                                void*           restrict  output,
                                GeglAbyssPolicy           repeat_mode);

G_DEFINE_TYPE (GeglSamplerNearest, gegl_sampler_nearest, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_nearest_class_init (GeglSamplerNearestClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get = gegl_sampler_nearest_get;
}

/*
 * It would seem that x=y=0 and width=height=1 should be enough, but
 * apparently safety w.r.t. round off or something else makes things
 * work better with width=height=3 and centering.
 */
static void
gegl_sampler_nearest_init (GeglSamplerNearest *self)
{
  GEGL_SAMPLER (self)->context_rect[0].x = 0;
  GEGL_SAMPLER (self)->context_rect[0].y = 0;
  GEGL_SAMPLER (self)->context_rect[0].width = 1;
  GEGL_SAMPLER (self)->context_rect[0].height = 1;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

void
gegl_sampler_nearest_get (      GeglSampler*    restrict  self,
                          const gdouble                   absolute_x,
                          const gdouble                   absolute_y,
                                GeglMatrix2              *scale,
                                void*           restrict  output,
                                GeglAbyssPolicy           repeat_mode)
{
  /*
   * The reason why floor of the absolute position gives the nearest
   * pixel (with ties resolved toward -infinity) is that the absolute
   * position is corner-based (origin at the top left corner of the
   * pixel labeled (0,0)) so that the center of the top left pixel is
   * located at (.5,.5) (instead of (0,0) as it would be if absolute
   * positions were center-based).
   */
  const gint channels = 4;
  gfloat newval[channels];
  const gfloat* restrict in_bptr =
    gegl_sampler_get_ptr (self,
                          (gint) floor ((double) absolute_x),
                          (gint) floor ((double) absolute_y),
                          repeat_mode);
  newval[0] = *in_bptr++;
  newval[1] = *in_bptr++;
  newval[2] = *in_bptr++;
  newval[3] = *in_bptr;
  babl_process (self->fish, newval, output, 1);
}
