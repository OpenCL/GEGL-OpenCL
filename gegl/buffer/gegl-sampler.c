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
#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-lanczos.h"

#if ENABLE_MP
GStaticRecMutex mutex = G_STATIC_REC_MUTEX_INIT;
#endif

enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_FORMAT,
  PROP_CONTEXT_RECT,
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
static void set_buffer              (GeglSampler  *self,
                                     GeglBuffer   *buffer);

G_DEFINE_TYPE (GeglSampler, gegl_sampler, G_TYPE_OBJECT)

static void
gegl_sampler_class_init (GeglSamplerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;
  object_class->dispose = dispose;

  klass->prepare = NULL;
  klass->get     = NULL;
  klass->set_buffer   = set_buffer;

  object_class->set_property = set_property;
  object_class->get_property = get_property;


  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format",
                                                         "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        "Buffer",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
gegl_sampler_init (GeglSampler *self)
{
  GeglRectangle context_rect = {0,0,1,1};
  GeglRectangle sampler_rectangle = {0,0,0,0};
  self->sampler_buffer = NULL;
  self->buffer = NULL;
  self->context_rect = context_rect;
  self->sampler_rectangle = sampler_rectangle;
}

void
gegl_sampler_get (GeglSampler *self,
                  gdouble      x,
                  gdouble      y,
                  void        *output)
{
  GeglSamplerClass *klass;
#if 0  /* avoiding expensive typecheck here */
  g_return_if_fail (GEGL_IS_SAMPLER (self));
#endif

  self->x = x;
  self->y = y;
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

void
gegl_sampler_set_buffer (GeglSampler *self, GeglBuffer *buffer)
{
  GeglSamplerClass *klass;

  g_return_if_fail (GEGL_IS_SAMPLER (self));

  klass = GEGL_SAMPLER_GET_CLASS (self);

  if (klass->set_buffer)
    klass->set_buffer (self, buffer);
}


static void
finalize (GObject *gobject)
{
  GeglSampler *sampler = GEGL_SAMPLER (gobject);
  if (sampler->sampler_buffer)
    {
      g_free (sampler->sampler_buffer);
      sampler->sampler_buffer = NULL;
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

#include <math.h>

gfloat *
gegl_sampler_get_from_buffer (GeglSampler *sampler,
                              gint         x,
                              gint         y)
{
   guchar        *buffer_ptr;
   gint           dx;
   gint           dy;
   gint           bpp;
   gint           sof;


   bpp = sampler->interpolate_format->format.bytes_per_pixel;

   if (sampler->sampler_buffer == NULL ||
       x < sampler->sampler_rectangle.x ||
       y < sampler->sampler_rectangle.y ||
       x >= sampler->sampler_rectangle.x + sampler->sampler_rectangle.width ||
       y >= sampler->sampler_rectangle.y + sampler->sampler_rectangle.height)
     {
       GeglRectangle  fetch_rectangle/* = sampler->context_rect*/;

       fetch_rectangle.x = (gint) sampler->x;
       fetch_rectangle.y = (gint) sampler->y;

       /* we override the fetch rectangle needed by the sampler, hoping that
        * the extra pixels we fetch comes in useful in subsequent requests,
        * we assume that it is more likely that further access is to the right
        * or down of our currently requested position.
        */
       fetch_rectangle.x -= 8;
       fetch_rectangle.y -= 8;
       fetch_rectangle.width = 64;
       fetch_rectangle.height = 64;


       if (sampler->sampler_buffer == NULL )
         { /* we always request the same amount of pixels (64kb worth) */
           sampler->sampler_buffer = g_malloc0 (fetch_rectangle.width *
                                                fetch_rectangle.height *
                                                bpp);
         }

       gegl_buffer_get (sampler->buffer,
                        1.0,
                        &fetch_rectangle,
                        sampler->interpolate_format,
                        sampler->sampler_buffer,
                        GEGL_AUTO_ROWSTRIDE);

       sampler->sampler_rectangle = fetch_rectangle;
     }

   dx = x - sampler->sampler_rectangle.x;
   dy = y - sampler->sampler_rectangle.y;
   buffer_ptr = (guchar *)sampler->sampler_buffer;
   sof = ( dx +  (dy * sampler->sampler_rectangle.width)) * bpp;
   return (gfloat*)(buffer_ptr+sof);
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

      default:
        break;
    }
}


static void
set_buffer (GeglSampler *self, GeglBuffer *buffer)
{
   if (self->buffer != buffer)
     {
        if (GEGL_IS_BUFFER(self->buffer))
          g_object_unref(self->buffer);
        if (GEGL_IS_BUFFER (buffer))
          self->buffer = gegl_buffer_dup (buffer);
        else
          self->buffer = NULL;
     }
}

GeglInterpolation
gegl_buffer_interpolation_from_string (const gchar *string)
{
  if (g_str_equal (string, "nearest") ||
      g_str_equal (string, "none"))
    return GEGL_INTERPOLATION_NEAREST;

  if (g_str_equal (string, "linear") ||
      g_str_equal (string, "bilinear"))
    return GEGL_INTERPOLATION_LINEAR;

  if (g_str_equal (string, "cubic") ||
      g_str_equal (string, "bicubic"))
    return GEGL_INTERPOLATION_CUBIC;

  if (g_str_equal (string, "lanczos"))
    return GEGL_INTERPOLATION_LANCZOS;

  return GEGL_INTERPOLATION_NEAREST;
}


GType
gegl_sampler_type_from_interpolation (GeglInterpolation interpolation)
{
  switch (interpolation)
    {
      case GEGL_INTERPOLATION_NEAREST:
        return GEGL_TYPE_SAMPLER_NEAREST;
      case GEGL_INTERPOLATION_LINEAR:
        return GEGL_TYPE_SAMPLER_LINEAR;
      case GEGL_INTERPOLATION_CUBIC:
        return GEGL_TYPE_SAMPLER_CUBIC;
      case GEGL_INTERPOLATION_LANCZOS:
        return GEGL_TYPE_SAMPLER_LANCZOS;
      default:        
        return GEGL_TYPE_SAMPLER_LINEAR;
    }
}
