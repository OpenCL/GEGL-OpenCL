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

#define SIZE 16  /* the cached region around a fetched pixel value is
                    SIZE×SIZE pixels. */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-interpolator.h"
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

static void gegl_interpolator_class_init (GeglInterpolatorClass *klass);
static void gegl_interpolator_init       (GeglInterpolator *self);
static void finalize                     (GObject *gobject);
static void dispose                      (GObject *gobject);
static void get_property                 (GObject    *gobject,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec);
static void set_property                 (GObject      *gobject,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);

G_DEFINE_TYPE (GeglInterpolator, gegl_interpolator, G_TYPE_OBJECT)

static void
gegl_interpolator_class_init (GeglInterpolatorClass *klass)
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
gegl_interpolator_init (GeglInterpolator *self)
{
  self->cache_buffer = NULL;
  self->buffer = NULL;
  self->context_pixels = 0;
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
  GeglInterpolator *interpolator = GEGL_INTERPOLATOR (gobject);
  if (interpolator->cache_buffer)
    {
      g_free (interpolator->cache_buffer);
      interpolator->cache_buffer = NULL;
    }
  G_OBJECT_CLASS (gegl_interpolator_parent_class)->finalize (gobject);
}

static void
dispose (GObject *gobject)
{
  GeglInterpolator *interpolator = GEGL_INTERPOLATOR (gobject);
  if (interpolator->buffer)
    {
      g_object_unref (interpolator->buffer);
      interpolator->buffer = NULL;
    }
  G_OBJECT_CLASS (gegl_interpolator_parent_class)->dispose (gobject);
}

void
gegl_interpolator_fill_buffer (GeglInterpolator *interpolator,
                               gdouble           x,
                               gdouble           y)
{
  GeglBuffer    *buffer;
  GeglRectangle  surround;
 
  buffer = interpolator->buffer;
  g_assert (buffer);

  if (interpolator->cache_buffer) 
    {
      GeglRectangle r = interpolator->cache_rectangle;

      /* check if the cache-buffer includes both the desired coordinates and
       * a sufficient surrounding context 
       */
      if (x - r.x >= interpolator->context_pixels &&
          x - r.x < r.width - interpolator->context_pixels &&
          y - r.y >= interpolator->context_pixels &&
          y - r.y < r.height - interpolator->context_pixels)
        {
          return;  /* we can reuse our cached interpolation source buffer */
        }

      g_free (interpolator->cache_buffer);
      interpolator->cache_buffer = NULL;
    }

  surround.x = x - SIZE/2;
  surround.y = y - SIZE/2;
  surround.width  = SIZE;
  surround.height = SIZE;

  interpolator->cache_buffer = g_malloc0 (surround.width *
                                          surround.height *
                                          4 * sizeof (gfloat));
  interpolator->cache_rectangle = surround;
  interpolator->interpolate_format = babl_format ("RaGaBaA float");

  /* XXX: why is this needed? it doesn't really make sense */
  surround.x += buffer->x;
  surround.y += buffer->y;

  gegl_buffer_get (buffer, &surround, 1.0,
                   interpolator->interpolate_format,
                   interpolator->cache_buffer);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglInterpolator *self = GEGL_INTERPOLATOR (object);

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
  GeglInterpolator *self = GEGL_INTERPOLATOR (object);

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
