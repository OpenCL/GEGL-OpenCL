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
#include <string.h>
#include <math.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-nearest.h"


enum
{
  PROP_0,
  PROP_LAST
};

static void
gegl_sampler_nearest_get (GeglSampler    *self,
                          gdouble         x,
                          gdouble         y,
                          GeglMatrix2    *scale,
                          void           *output,
                          GeglAbyssPolicy repeat_mode);

G_DEFINE_TYPE (GeglSamplerNearest, gegl_sampler_nearest, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_nearest_class_init (GeglSamplerNearestClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get = gegl_sampler_nearest_get;
}

static void
gegl_sampler_nearest_init (GeglSamplerNearest *self)
{
  GEGL_SAMPLER (self)->context_rect[0].x = 0;
  GEGL_SAMPLER (self)->context_rect[0].y = 0;
  GEGL_SAMPLER (self)->context_rect[0].width = 1;
  GEGL_SAMPLER (self)->context_rect[0].height = 1;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RGBA float");
}

void
gegl_sampler_nearest_get (GeglSampler     *self,
                          gdouble          x,
                          gdouble          y,
                          GeglMatrix2     *scale,
                          void            *output,
                          GeglAbyssPolicy  repeat_mode)
{
  gfloat *sampler_bptr;

  sampler_bptr = gegl_sampler_get_from_buffer (self,
                                               (gint) floor ((double) x),
                                               (gint) floor ((double) y),
                                               repeat_mode);
  babl_process (self->fish, sampler_bptr, output, 1);
}
