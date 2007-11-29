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
#include "gegl-sampler-linear.h"
#include "gegl-buffer-private.h"
#include <string.h>
enum
{
  PROP_0,
  PROP_CONTEXT_PIXELS,
  PROP_LAST
};

static void    gegl_sampler_linear_get (GeglSampler *self,
                                        gdouble           x,
                                        gdouble           y,
                                        void             *output);
static void    set_property            (GObject      *gobject,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);
static void    get_property            (GObject    *gobject,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec);


G_DEFINE_TYPE (GeglSamplerLinear, gegl_sampler_linear, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_linear_class_init (GeglSamplerLinearClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get     = gegl_sampler_linear_get;

  g_object_class_install_property (object_class, PROP_CONTEXT_PIXELS,
                                   g_param_spec_int ("context-pixels",
                                                     "ContextPixels",
                                                     "number of neighbourhood pixels needed in each direction",
                                                     0, 16, 1,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


}

static void
gegl_sampler_linear_init (GeglSamplerLinear *self)
{
  GEGL_SAMPLER (self)->context_pixels=1;
}

void
gegl_sampler_linear_get (GeglSampler *sampler,
                              gdouble           x,
                              gdouble           y,
                              void             *output)
{
  GeglRectangle *rect;
  gfloat        *cache_buffer;
  gfloat         dst[4];
  gfloat         abyss = 0.;

  gegl_sampler_fill_buffer (sampler, x, y);
  rect = &sampler->cache_rectangle;
  cache_buffer = sampler->cache_buffer;
  if (!cache_buffer)
    return;

  if (x >= rect->x &&
      y >= rect->y &&
      x < rect->x+rect->width &&
      y < rect->y+rect->height)
    {
      x -= rect->x;
      y -= rect->y;

      gint    i;
      gint    x0 = 0;
      gint    y0 = 0;
      gint    x1 = rect->width - 1;
      gint    y1 = rect->height - 1;

      gint    u  = (gint) x;
      gint    v  = (gint) y;
      gdouble uf = x - u;
      gdouble vf = y - v;

      gdouble q1 = (1 - uf) * (1 - vf);
      gdouble q2 = uf * (1 - vf);
      gdouble q3 = (1 - uf) * vf;
      gdouble q4 = uf * vf;

      gfloat *p00 = cache_buffer + (v * rect->width + u) * 4;
      u = CLAMP ((gint) x + 0, x0, x1);
      v = CLAMP ((gint) y + 1, y0, y1);
      gfloat *p01 = cache_buffer + (v * rect->width + u) * 4;
      u = CLAMP ((gint) x + 1, x0, x1);
      v = CLAMP ((gint) y + 0, y0, y1);
      gfloat *p10 = cache_buffer + (v * rect->width + u) * 4;
      u = CLAMP ((gint) x + 1, x0, x1);
      v = CLAMP ((gint) y + 1, y0, y1);
      gfloat *p11 = cache_buffer + (v * rect->width + u) * 4;

      for (i = 0; i < 4; i++)
        {
          dst[i] = q1 * p00[i] + q2 * p10[i] + q3 * p01[i] + q4 * p11[i];
        }
    }
  else
    {
      dst[0] = abyss;
      dst[1] = abyss;
      dst[2] = abyss;
      dst[3] = abyss;
    }
  babl_process (babl_fish (sampler->interpolate_format, sampler->format),
                dst, output, 1);
}



static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
      case PROP_CONTEXT_PIXELS:
        g_object_set_property (gobject, "GeglSampler::context-pixels", value);
        break;

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
      case PROP_CONTEXT_PIXELS:
        g_object_get_property (gobject, "GeglSampler::context-pixels", value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

