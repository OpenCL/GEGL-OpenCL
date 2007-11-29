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
#include "gegl-sampler-nearest.h"
#include "gegl-buffer-private.h"
#include <string.h>

static void    gegl_sampler_nearest_get (GeglSampler *self,
                                         gdouble      x,
                                         gdouble      y,
                                         void        *output);


G_DEFINE_TYPE (GeglSamplerNearest, gegl_sampler_nearest, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_nearest_class_init (GeglSamplerNearestClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get     = gegl_sampler_nearest_get;

}

static void
gegl_sampler_nearest_init (GeglSamplerNearest *self)
{
}

void
gegl_sampler_nearest_get (GeglSampler *self,
                          gdouble      x,
                          gdouble      y,
                          void        *output)
{
  gfloat        *cache_buffer;
  GeglRectangle *rect;
  gfloat         dst[4];

  gegl_sampler_fill_buffer (self, x, y);

  rect = &self->cache_rectangle;
  cache_buffer = self->cache_buffer;
  if (!cache_buffer)
    return;

  gint u = (x - rect->x);
  gint v = (y - rect->y);

  if (u >= 0 &&
      v >= 0 &&
      u < rect->width &&
      v < rect->height)
    {
      gint pos = (v * rect->width + u) * 4;
      memcpy (dst, cache_buffer + pos, sizeof (gfloat) * 4);
    }
  else
    {
      dst[0] = 0.0;
      dst[1] = 0.0;
      dst[2] = 0.0;
      dst[3] = 0.0;
    }
  babl_process (babl_fish (self->interpolate_format, self->format),
                dst, output, 1);
}

