/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-interpolator.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"

static void      gegl_interpolator_class_init (GeglInterpolatorClass *klass);
static void      gegl_interpolator_init (GeglInterpolator *self);
static void      finalize (GObject *gobject);

G_DEFINE_TYPE (GeglInterpolator, gegl_interpolator, G_TYPE_OBJECT)

static void
gegl_interpolator_class_init (GeglInterpolatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;

  klass->prepare = NULL;
  klass->get     = NULL;
}

static void
gegl_interpolator_init (GeglInterpolator *self)
{
  self->cache_buffer = NULL;
}

void
gegl_interpolator_get (GeglInterpolator *self,
                       gdouble           x,
                       gdouble           y,
                       void             *output)
{
  GeglInterpolatorClass *klass;

  g_return_if_fail (GEGL_IS_INTERPOLATOR (self));

  klass = GEGL_INTERPOLATOR_GET_CLASS (self);

  klass->get (self, x, y, output);
}

void
gegl_interpolator_prepare (GeglInterpolator *self)
{
  GeglInterpolatorClass *klass;

  g_return_if_fail (GEGL_IS_INTERPOLATOR (self));

  klass = GEGL_INTERPOLATOR_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self);

  if (self->cache_buffer) /* to force a regetting of the region, even
                             if the cached getter might be valid
                           */
    {
      g_free (self->cache_buffer);
      self->cache_buffer = NULL;
    }
}

static void
finalize (GObject *gobject)
{
  GeglInterpolator *interpolator = GEGL_INTERPOLATOR (gobject);
  if (interpolator->cache_buffer)
    {
      g_free (interpolator->cache_buffer);
    }
  G_OBJECT_CLASS (gegl_interpolator_parent_class)->finalize (gobject);
}

void
gegl_interpolator_fill_buffer (GeglInterpolator *interpolator,
                               gdouble           x,
                               gdouble           y)
{
  GeglBuffer *input;
 
  input = interpolator->input;
  g_assert (input);

  if (interpolator->cache_buffer) /* FIXME: check that the geometry of the
                                            existing cache is good for the
                                            provided coordinates as well
                                   */
    {
      GeglRectangle r = interpolator->cache_rectangle;
      /* FIXME: take sampling radius into account */

      if (x>=r.x && x<r.x+r.width &&
          y>=r.y && y<r.y+r.height)
          /* we're already prefetched for the correct data */
          return;

      /* requested sample outside defined region, bailing early */
      if (x < input->x ||
          x > input->x+input->width ||
          y < input->height ||
          y > input->y+input->height)
        return;
      /*return;*/

      g_free (interpolator->cache_buffer);
    }

  /* by default we just grab everything for the interpolator 
   *
   * FIXME: do not grab the whole GeglBuffer but only a fixed amount of
   * pixels to allow this infrastructure to scale.
   * */
  interpolator->cache_buffer = g_malloc0 (input->width * input->height * 4 * 4);
  interpolator->cache_rectangle = * gegl_buffer_extent (input);
  interpolator->interpolate_format = babl_format ("RaGaBaA float");
  gegl_buffer_get (interpolator->input, NULL, 1.0,
                   interpolator->interpolate_format,
                   interpolator->cache_buffer);
}
