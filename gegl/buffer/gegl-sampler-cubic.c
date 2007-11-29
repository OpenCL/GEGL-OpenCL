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
 */
#include "gegl-sampler-cubic.h"
#include "gegl-buffer-private.h" /* XXX */
#include <string.h>
#include <math.h>

/* XXX WARNING: This code compiles, but is functionally broken, and
 * currently not used by the rest of GeglBuffer */

enum
{
  PROP_0,
  PROP_B,
  PROP_TYPE,
  PROP_LAST
};

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static inline float cubicKernel(float x, float b, float c);
static void    finalize                         (GObject       *gobject);

static void    gegl_sampler_cubic_get     (GeglSampler *self,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 void             *output);

static void    gegl_sampler_cubic_prepare (GeglSampler *self);


G_DEFINE_TYPE (GeglSamplerCubic, gegl_sampler_cubic, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_cubic_class_init (GeglSamplerCubicClass *klass)
{
  GObjectClass          *object_class       = G_OBJECT_CLASS (klass);
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  object_class->finalize     = finalize;
  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->prepare = gegl_sampler_cubic_prepare;
  sampler_class->get     = gegl_sampler_cubic_get;

  g_object_class_install_property (object_class, PROP_B,
                                   g_param_spec_double ("b",
                                                        "B",
                                                        "B-spline parameter",
                                                        0.0,
                                                        1.0,
                                                        0.0,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_string ("type",
                                                        "type",
                                                        "B-spline type (cubic | catmullrom | formula) 2c+b=1",
                                                        "cubic",
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
gegl_sampler_cubic_init (GeglSamplerCubic *self)
{
}

void
gegl_sampler_cubic_prepare (GeglSampler *sampler)
{
  GeglSamplerCubic *self  = GEGL_SAMPLER_CUBIC (sampler);

  /* fill the internal bufer */

  if (strcmp (self->type, "cubic"))
    {
      /* cubic B-spline */
      self->b = 0.0;
      self->c = 0.5;
    }
  else if (strcmp (self->type, "catmullrom"))
    {
      /* Catmull-Rom spline */
      self->b = 1.0;
      self->c = 0.0;
    }
  else if (strcmp (self->type, "formula"))
    {
      self->c = (1 - self->b) / 2.0;
    }
}

static void
finalize (GObject *object)
{
  GeglSamplerCubic *self         = GEGL_SAMPLER_CUBIC (object);

  if (self->type)
    g_free (self->type);
  G_OBJECT_CLASS (gegl_sampler_cubic_parent_class)->finalize (object);
}

void
gegl_sampler_cubic_get (GeglSampler *sampler,
                             gdouble           x,
                             gdouble           y,
                             void             *output)
{
  GeglSamplerCubic *self      = GEGL_SAMPLER_CUBIC (sampler);
  gfloat                *cache_buffer = sampler->cache_buffer;
  GeglRectangle         *rectangle = &sampler->cache_rectangle;
  GeglBuffer            *buffer    = sampler->buffer;
  gfloat                *buf_ptr;
  gfloat                 factor;

  gdouble                arecip;
  gdouble                newval[4];

  gfloat                 dst[4];
  gfloat                 abyss = 0.;
  gint                   i, j, pu, pv;

  gegl_sampler_fill_buffer (sampler, x, y);
  cache_buffer = sampler->cache_buffer;
  if (!buffer)
    return;

  if (x >= 0 &&
      y >= 0 &&
      x < buffer->extent.width &&
      y < buffer->extent.height)
    {
      gint u = (gint) (x - rectangle->x);
      gint v = (gint) (y - rectangle->y);
      newval[0] = newval[1] = newval[2] = newval[3] = 0.0;
      for (j = -1; j <= 2; j++)
        for (i = -1; i <= 2; i++)
          {
            pu         = CLAMP (u + i, 0, rectangle->width - 1);
            pv         = CLAMP (v + j, 0, rectangle->height - 1);
            factor     = cubicKernel ((y - rectangle->y) - pv, self->b, self->c) *
                         cubicKernel ((x - rectangle->x) - pu, self->b, self->c);
            buf_ptr    = cache_buffer + ((pv * rectangle->width + pu) * 4);
            newval[0] += factor * buf_ptr[0] * buf_ptr[3];
            newval[1] += factor * buf_ptr[1] * buf_ptr[3];
            newval[2] += factor * buf_ptr[2] * buf_ptr[3];
            newval[3] += factor * buf_ptr[3];
          }
      if (newval[3] <= 0.0)
        {
          arecip    = 0.0;
          newval[3] = 0;
        }
      else if (newval[3] > G_MAXDOUBLE)
        {
          arecip    = 1.0 / newval[3];
          newval[3] = G_MAXDOUBLE;
        }
      else
        {
          arecip = 1.0 / newval[3];
        }

      dst[0] = CLAMP (newval[0] * arecip, 0, G_MAXDOUBLE);
      dst[1] = CLAMP (newval[1] * arecip, 0, G_MAXDOUBLE);
      dst[2] = CLAMP (newval[2] * arecip, 0, G_MAXDOUBLE);
      dst[3] = CLAMP (newval[3], 0, G_MAXDOUBLE);
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
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSamplerCubic *self         = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        g_value_set_double (value, self->b);
        break;

      case PROP_TYPE:
        g_value_set_string (value, self->type);
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSamplerCubic *self         = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        self->b = g_value_get_double (value);
        break;

      case PROP_TYPE:
      {
        const gchar *type = g_value_get_string (value);
        if (self->type)
          g_free (self->type);
        self->type = g_strdup (type);
        break;
      }

      default:
        break;
    }
}

static inline float cubicKernel (float x, float b, float c)
{
  float weight, x2, x3;
  float ax = (float) fabs (x);

  if (ax > 2) return 0;

  x3 = ax * ax * ax;
  x2 = ax * ax;

  if (ax < 1)
    weight = (12 - 9 * b - 6 * c) * x3 +
             (-18 + 12 * b + 6 * c) * x2 +
             (6 - 2 * b);
  else
    weight = (-b - 6 * c) * x3 +
             (6 * b + 30 * c) * x2 +
             (-12 * b - 48 * c) * ax +
             (8 * b + 24 * c);

  return weight * (1.0 / 6.0);
}
