/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * 2007 © Øyvind Kolås
 */
#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-lanczos.h"
#include "gegl-sampler-downsharp.h"
#include "gegl-sampler-downsize.h"
#include "gegl-sampler-downsmooth.h"
#include "gegl-sampler-upsharp.h"
#include "gegl-sampler-upsize.h"
#include "gegl-sampler-upsmooth.h"

enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_FORMAT,
  PROP_CONTEXT_RECT,
  PROP_INVERSE_JACOBIAN,
  PROP_LAST
};

static void gegl_sampler_class_init (GeglSamplerClass *klass);

static void gegl_sampler_init (GeglSampler *self);

static void finalize (GObject *gobject);

static void dispose (GObject *gobject);

static void get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec);

static void set_property (      GObject      *gobject,
                                guint         property_id,
                          const GValue *value,
                                GParamSpec   *pspec);

static void set_buffer (GeglSampler  *self,
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

  g_object_class_install_property (
                 object_class,
                 PROP_FORMAT,
                 g_param_spec_pointer ("format",
                                       "format",
                                       "babl format",
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (
                   object_class,
                   PROP_BUFFER,
                   g_param_spec_object ("buffer",
                                        "Buffer",
                                        "Input pad, for image buffer input.",
                                        GEGL_TYPE_BUFFER,
                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (
                 object_class,
                 PROP_INVERSE_JACOBIAN,
                 g_param_spec_pointer ("inverse_jacobian",
                                       "Inverse Jacobian",
                                       "Inverse Jacobian matrix, for certain samplers.",
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
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


  self->fish = babl_fish (self->interpolate_format, self->format);

  /*
   * This makes the cache rect invalid, in case the data in the buffer
   * has changed:
   */
  self->sampler_rectangle.width = 0;
  self->sampler_rectangle.height = 0;

#if 0
  if (self->cache_buffer) /* Force a regetting of the region, even
                             though the cached getter may be valid. */
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

/*
 * Gets a pointer to the center pixel, within a buffer that has a
 * rowstride of 64px * 16bpp:
 */
gfloat *
gegl_sampler_get_ptr (GeglSampler *const sampler,
                      const gint         x,
                      const gint         y)
{
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gint bpp =
    babl_format_get_bytes_per_pixel (sampler->interpolate_format);

  /*
   * maximum_width_and_height is the largest number of pixels which
   * can be be requested in the horizontal or vertical directions (64
   * in GEGL).
   */
  const gint maximum_width_and_height = 64;
  g_assert (sampler->context_rect.width  <= maximum_width_and_height);
  g_assert (sampler->context_rect.height <= maximum_width_and_height);

  if (( sampler->sampler_buffer == NULL )
      ||
      ( x + sampler->context_rect.x < sampler->sampler_rectangle.x )
      ||
      ( y + sampler->context_rect.y < sampler->sampler_rectangle.y )
      ||
      ( x + sampler->context_rect.x + sampler->context_rect.width
        >= sampler->sampler_rectangle.x + sampler->sampler_rectangle.width )
      ||
      ( y + sampler->context_rect.y + sampler->context_rect.height
        >= sampler->sampler_rectangle.y + sampler->sampler_rectangle.height ))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle:
       */
      GeglRectangle fetch_rectangle;

      /*
       * Override the fetch rectangle needed by the sampler, hoping
       * that the extra pixels are useful for subsequent requests,
       * assuming that it is more likely that further access is to the
       * right or down of our currently requested
       * position. Consequently, we move the top left corner of the
       * context_rect by about one fourth of the maximal distance we
       * can (one fourth of one half = one eight). Given that the
       * maximum width and height of the fetch_rectangle is 64, so
       * that half of it is 32, one fourth of the elbow room is at
       * most 8. If context_rect is large, the corner is not moved
       * much if at all, as should be.
       */
      fetch_rectangle.x =
        x + sampler->context_rect.x
        - ( maximum_width_and_height - sampler->context_rect.width  ) / 8;
      fetch_rectangle.y =
        y + sampler->context_rect.y
        - ( maximum_width_and_height - sampler->context_rect.height ) / 8;

      fetch_rectangle.width  = maximum_width_and_height;
      fetch_rectangle.height = maximum_width_and_height;

      if (sampler->sampler_buffer == NULL)
        {
          /*
           * Always request the same amount of pixels:
           */
          sampler->sampler_buffer =
            g_malloc0 (( maximum_width_and_height * maximum_width_and_height )
                       * bpp);
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
  sof = ( dx + dy * sampler->sampler_rectangle.width ) * bpp;

  return (gfloat*)(buffer_ptr+sof);
}

gfloat *
gegl_sampler_get_from_buffer (GeglSampler *const sampler,
                              const gint         x,
                              const gint         y)
{
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gint bpp =
    babl_format_get_bytes_per_pixel (sampler->interpolate_format);

  /*
   * maximum_width_and_height is the largest number of pixels which
   * can be be requested in the horizontal or vertical directions (64
   * in GEGL).
   */
  const gint maximum_width_and_height = 64;
  g_assert (sampler->context_rect.width  <= maximum_width_and_height);
  g_assert (sampler->context_rect.height <= maximum_width_and_height);

  if (( sampler->sampler_buffer == NULL )
      ||
      ( x < sampler->sampler_rectangle.x )
      ||
      ( y < sampler->sampler_rectangle.y )
      ||
      ( x >= sampler->sampler_rectangle.x + sampler->sampler_rectangle.width )
      ||
      ( y >= sampler->sampler_rectangle.y + sampler->sampler_rectangle.height ))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle:
       */
      GeglRectangle fetch_rectangle;

      fetch_rectangle.x =
        x - ( maximum_width_and_height - sampler->context_rect.width  ) / 8;
      fetch_rectangle.y =
        y - ( maximum_width_and_height - sampler->context_rect.height ) / 8;

      fetch_rectangle.width  = maximum_width_and_height;
      fetch_rectangle.height = maximum_width_and_height;

      if (sampler->sampler_buffer == NULL)
        {
          /*
           * Always request the same amount of pixels:
           */
          sampler->sampler_buffer =
            g_malloc0 (( maximum_width_and_height * maximum_width_and_height )
                       * bpp);
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
  sof = ( dx + dy * sampler->sampler_rectangle.width ) * bpp;

  return (gfloat*)(buffer_ptr+sof);
}

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSampler *self = GEGL_SAMPLER (object);

  switch (property_id)
    {
      case PROP_BUFFER:
        g_value_set_object (value, self->buffer);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, self->format);
        break;

      case PROP_INVERSE_JACOBIAN:
        g_value_set_pointer (value, self->inverse_jacobian);
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSampler *self = GEGL_SAMPLER (object);

  switch (property_id)
    {
      case PROP_BUFFER:
        self->buffer = GEGL_BUFFER (g_value_dup_object (value));
        break;

      case PROP_FORMAT:
        self->format = g_value_get_pointer (value);
        break;

      case PROP_INVERSE_JACOBIAN:
        self->inverse_jacobian = g_value_get_pointer (value);
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

  if (g_str_equal (string, "downsharp"))
    return GEGL_INTERPOLATION_DOWNSHARP;

  if (g_str_equal (string, "downsize"))
    return GEGL_INTERPOLATION_DOWNSIZE;

  if (g_str_equal (string, "downsmooth"))
    return GEGL_INTERPOLATION_DOWNSMOOTH;

  if (g_str_equal (string, "upsharp"))
    return GEGL_INTERPOLATION_UPSHARP;

  if (g_str_equal (string, "upsize"))
    return GEGL_INTERPOLATION_UPSIZE;

  if (g_str_equal (string, "upsmooth"))
    return GEGL_INTERPOLATION_UPSMOOTH;

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
      case GEGL_INTERPOLATION_DOWNSHARP:
        return GEGL_TYPE_SAMPLER_DOWNSHARP;
      case GEGL_INTERPOLATION_DOWNSIZE:
        return GEGL_TYPE_SAMPLER_DOWNSIZE;
      case GEGL_INTERPOLATION_DOWNSMOOTH:
        return GEGL_TYPE_SAMPLER_DOWNSMOOTH;
      case GEGL_INTERPOLATION_UPSHARP:
        return GEGL_TYPE_SAMPLER_UPSHARP;
      case GEGL_INTERPOLATION_UPSIZE:
        return GEGL_TYPE_SAMPLER_UPSIZE;
      case GEGL_INTERPOLATION_UPSMOOTH:
        return GEGL_TYPE_SAMPLER_UPSMOOTH;
      default:
        return GEGL_TYPE_SAMPLER_LINEAR;
    }
}
