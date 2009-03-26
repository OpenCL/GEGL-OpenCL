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

static void    gegl_sampler_nearest_get (GeglSampler  *self,
                                         gdouble       x,
                                         gdouble       y,
                                         void         *output);
static void    set_property             (GObject      *gobject,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static void    get_property             (GObject      *gobject,
                                         guint         prop_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);

G_DEFINE_TYPE (GeglSamplerNearest, gegl_sampler_nearest, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_nearest_class_init (GeglSamplerNearestClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get     = gegl_sampler_nearest_get;

}

static void
gegl_sampler_nearest_init (GeglSamplerNearest *self)
{
   GEGL_SAMPLER (self)->context_rect.x = 0;
   GEGL_SAMPLER (self)->context_rect.y = 0;
   GEGL_SAMPLER (self)->context_rect.width = 1;
   GEGL_SAMPLER (self)->context_rect.height = 1;
   GEGL_SAMPLER (self)->interpolate_format = babl_format ("RGBA float");
}

void
gegl_sampler_nearest_get (GeglSampler *self,
                          gdouble      x,
                          gdouble      y,
                          void        *output)
{
  gfloat             *sampler_bptr;
  sampler_bptr = gegl_sampler_get_from_buffer (self, (gint)x, (gint)y);
  babl_process (babl_fish (self->interpolate_format, self->format), sampler_bptr, output, 1);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}
