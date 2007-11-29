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
 * 2007 © Øyvind Kolås
 */

#define SIZE 16  /* the cached region around a fetched pixel value is
                    SIZE×SIZE pixels. */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-sampler.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"

enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_FORMAT,
  PROP_CONTEXT_PIXELS,
  PROP_LAST
};

static void gegl_sampler_class_init (GeglSamplerClass *klass);
static void gegl_sampler_init       (GeglSampler *self);
static void finalize                (GObject *gobject);
static void dispose                 (GObject *gobject);
static void get_property            (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec);
static void set_property            (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec);

G_DEFINE_TYPE (GeglSampler, gegl_sampler, G_TYPE_OBJECT)

static void
gegl_sampler_class_init (GeglSamplerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;
  object_class->dispose = dispose;

  klass->prepare = NULL;
  klass->get     = NULL;

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        "Buffer",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format",
                                                         "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONTEXT_PIXELS,
                                   g_param_spec_int ("context-pixels",
                                                     "ContextPixels",
                                                     "number of neighbourhood pixels needed in each direction",
                                                     0, 16, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

}

static void
gegl_sampler_init (GeglSampler *self)
{
  self->cache_buffer = NULL;
  self->buffer = NULL;
  self->context_pixels = 0;
}

void
gegl_sampler_get (GeglSampler *self,
                       gdouble           x,
                       gdouble           y,
                       void             *output)
{
  GeglSamplerClass *klass;

  g_return_if_fail (GEGL_IS_SAMPLER (self));

  klass = GEGL_SAMPLER_GET_CLASS (self);

  klass->get (self, x, y, output);
}

void
gegl_sampler_prepare (GeglSampler *self)
{
  GeglSamplerClass *klass;

  g_return_if_fail (GEGL_IS_SAMPLER (self));

  klass = GEGL_SAMPLER_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self);

#if 0
  if (self->cache_buffer) /* to force a regetting of the region, even
                             if the cached getter might be valid
                           */
    {
      g_free (self->cache_buffer);
      self->cache_buffer = NULL;
    }
#endif
}

static void
finalize (GObject *gobject)
{
  GeglSampler *sampler = GEGL_SAMPLER (gobject);
  if (sampler->cache_buffer)
    {
      g_free (sampler->cache_buffer);
      sampler->cache_buffer = NULL;
    }
  G_OBJECT_CLASS (gegl_sampler_parent_class)->finalize (gobject);
}

static void
dispose (GObject *gobject)
{
  GeglSampler *sampler = GEGL_SAMPLER (gobject);
  if (sampler->buffer)
    {
      g_object_unref (sampler->buffer);
      sampler->buffer = NULL;
    }
  G_OBJECT_CLASS (gegl_sampler_parent_class)->dispose (gobject);
}

void
gegl_sampler_fill_buffer (GeglSampler *sampler,
                          gdouble      x,
                          gdouble      y)
{
  GeglBuffer    *buffer;
  GeglRectangle  surround;
 
  buffer = sampler->buffer;
  g_assert (buffer);

  if (sampler->cache_buffer) 
    {
      GeglRectangle r = sampler->cache_rectangle;

      /* check if the cache-buffer includes both the desired coordinates and
       * a sufficient surrounding context 
       */
      if (x - r.x >= sampler->context_pixels &&
          x - r.x < r.width - sampler->context_pixels &&
          y - r.y >= sampler->context_pixels &&
          y - r.y < r.height - sampler->context_pixels)
        {
          return;  /* we can reuse our cached interpolation source buffer */
        }

      g_free (sampler->cache_buffer);
      sampler->cache_buffer = NULL;
    }

  surround.x = x - SIZE/2;
  surround.y = y - SIZE/2;
  surround.width  = SIZE;
  surround.height = SIZE;

  sampler->cache_buffer = g_malloc0 (surround.width *
                                          surround.height *
                                          4 * sizeof (gfloat));
  sampler->cache_rectangle = surround;
  sampler->interpolate_format = babl_format ("RaGaBaA float");

  gegl_buffer_get (buffer, 1.0, &surround,
                   sampler->interpolate_format,
                   sampler->cache_buffer, GEGL_AUTO_ROWSTRIDE);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSampler *self = GEGL_SAMPLER (object);

  switch (prop_id)
    {
      case PROP_BUFFER:
        g_value_set_object (value, self->buffer);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, self->format);
        break;

      case PROP_CONTEXT_PIXELS:
        g_value_set_int (value, self->context_pixels);
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
  GeglSampler *self = GEGL_SAMPLER (object);

  switch (prop_id)
    {
      case PROP_BUFFER:
        self->buffer = GEGL_BUFFER (g_value_dup_object (value));
        break;

      case PROP_FORMAT:
        self->format = g_value_get_pointer (value);
        break;

      case PROP_CONTEXT_PIXELS:
        self->context_pixels = g_value_get_int (value);
        break;

      default:
        break;
    }
}
