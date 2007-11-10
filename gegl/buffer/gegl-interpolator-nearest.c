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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "gegl-interpolator-nearest.h"
#include "gegl-buffer-private.h"
#include <string.h>

static void    gegl_interpolator_nearest_get (GeglInterpolator *self,
                                              gdouble           x,
                                              gdouble           y,
                                              void             *output);


G_DEFINE_TYPE (GeglInterpolatorNearest, gegl_interpolator_nearest, GEGL_TYPE_INTERPOLATOR)

static void
gegl_interpolator_nearest_class_init (GeglInterpolatorNearestClass *klass)
{
  GeglInterpolatorClass *interpolator_class = GEGL_INTERPOLATOR_CLASS (klass);

  interpolator_class->get     = gegl_interpolator_nearest_get;

}

static void
gegl_interpolator_nearest_init (GeglInterpolatorNearest *self)
{
}

void
gegl_interpolator_nearest_get (GeglInterpolator *self,
                               gdouble           x,
                               gdouble           y,
                               void             *output)
{
  gfloat        *cache_buffer;
  GeglRectangle *rect;
  gfloat         dst[4];

  gegl_interpolator_fill_buffer (self, x, y);

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

