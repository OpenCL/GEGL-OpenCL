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
 * 2009,2012 © Nicolas Robidoux
 * 2011 © Adam Turcotte
 */
#include "config.h"

#include <glib-object.h>
#include <string.h>
#include <math.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer.h"
#include "gegl-utils.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-nohalo.h"
#include "gegl-sampler-lohalo.h"

enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_FORMAT,
  PROP_CONTEXT_RECT,
  PROP_LAST
};

static void gegl_sampler_class_init (GeglSamplerClass    *klass);

static void gegl_sampler_init       (GeglSampler         *self);

static void finalize                (GObject             *gobject);

static void dispose                 (GObject             *gobject);

static void get_property            (GObject             *gobject,
                                     guint                property_id,
                                     GValue              *value,
                                     GParamSpec          *pspec);

static void set_property            (GObject             *gobject,
                                     guint                property_id,
                                     const GValue        *value,
                                     GParamSpec          *pspec);

static void set_buffer              (GeglSampler         *self,
                                     GeglBuffer          *buffer);

static void buffer_contents_changed (GeglBuffer          *buffer,
                                     const GeglRectangle *changed_rect,
                                     gpointer             userdata);

GType gegl_sampler_gtype_from_enum  (GeglSamplerType      sampler_type);

G_DEFINE_TYPE (GeglSampler, gegl_sampler, G_TYPE_OBJECT)

static void
gegl_sampler_class_init (GeglSamplerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;
  object_class->dispose  = dispose;

  klass->prepare    = NULL;
  klass->get        = NULL;
  klass->set_buffer = set_buffer;

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
}

static void
gegl_sampler_init (GeglSampler *self)
{
  int i = 0;
  self->buffer = NULL;
  do {
    GeglRectangle context_rect      = {0,0,1,1};
    GeglRectangle sampler_rectangle = {0,0,0,0};
    self->sampler_buffer[i]         = NULL;
    self->context_rect[i]           = context_rect;
    self->sampler_rectangle[i]      = sampler_rectangle;
  } while ( ++i<GEGL_SAMPLER_MIPMAP_LEVELS );
}

void
gegl_sampler_get (GeglSampler     *self,
                  gdouble          x,
                  gdouble          y,
                  GeglMatrix2     *scale,
                  void            *output,
                  GeglAbyssPolicy  repeat_mode)
{
  self->get (self, x, y, scale, output, repeat_mode);
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
  self->sampler_rectangle[0].width = 0;
  self->sampler_rectangle[0].height = 0;

#if 0
  if (self->cache_buffer) /* Force a regetting of the region, even
                             though the cached getter may be valid. */
    {
      g_free (self->cache_buffer);
      self->cache_buffer = NULL;
    }
#endif
  self->get = klass->get; /* cache the sampler in the instance */
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
  int i = 0;
  do {
    if (sampler->sampler_buffer[i])
      {
        g_free (sampler->sampler_buffer[i]);
        sampler->sampler_buffer[i] = NULL;
      }
  } while ( ++i<GEGL_SAMPLER_MIPMAP_LEVELS );
  G_OBJECT_CLASS (gegl_sampler_parent_class)->finalize (gobject);
}

static void
dispose (GObject *gobject)
{
  GeglSampler *sampler = GEGL_SAMPLER (gobject);

  /* This call handles unreffing the buffer and disconnecting signals */
  set_buffer (sampler, NULL);

  G_OBJECT_CLASS (gegl_sampler_parent_class)->dispose (gobject);
}

/*
 * Gets a pointer to the center pixel, within a buffer that has a
 * rowstride of GEGL_SAMPLER_MAXIMUM_WIDTH px * bpp.
 */
gfloat *
gegl_sampler_get_ptr (GeglSampler *const sampler,
                      const gint         x,
                      const gint         y,
                      GeglAbyssPolicy    repeat_mode)
{
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gint bpp =
    babl_format_get_bytes_per_pixel (sampler->interpolate_format);

  const gint maximum_width  = GEGL_SAMPLER_MAXIMUM_WIDTH;
  const gint maximum_height = GEGL_SAMPLER_MAXIMUM_HEIGHT;
  g_assert (sampler->context_rect[0].width  <= maximum_width);
  g_assert (sampler->context_rect[0].height <= maximum_height);

  if ((sampler->sampler_buffer[0] == NULL)
      ||
      (x + sampler->context_rect[0].x < sampler->sampler_rectangle[0].x)
      ||
      (y + sampler->context_rect[0].y < sampler->sampler_rectangle[0].y)
      ||
      (x + sampler->context_rect[0].x + sampler->context_rect[0].width >
       sampler->sampler_rectangle[0].x + sampler->sampler_rectangle[0].width)
      ||
      (y + sampler->context_rect[0].y + sampler->context_rect[0].height >
       sampler->sampler_rectangle[0].y + sampler->sampler_rectangle[0].height))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle[0]:
       */
      GeglRectangle fetch_rectangle;

      /*
       * Always request the same amount of pixels:
       */
      if (sampler->sampler_buffer[0] == NULL)
        {
          sampler->sampler_buffer[0] =
            g_malloc0 (maximum_width * maximum_height * bpp);
        }

      /*
       * Override the fetch rectangle needed by the sampler in order
       * to prevent constant buffer creation, adding some elbow room
       * at the top and left so that buffers are not constantly
       * re-created when things are "slanted the wrong way", taking
       * into account that it is more likely that further access is to
       * the right or down of our currently requested
       * position. Consequently, we move the top left corner of the
       * context_rect, which has the effect of leaving elbow room there.
       *
       * Note that transform-core now works hard to have scanlines go
       * toward the interior of the buffer.
       *
       * For operations like resize and left-right and top-bottom
       * reflections, there is no reason to add elbow room. This means
       * that the following code will need to have a dual personality
       * sooner or later. Eliminating the elbow room for such
       * operations (and making tiles elongated rectangles) gives a
       * big speedup.
       */
      fetch_rectangle.width  = maximum_width;
      fetch_rectangle.height = maximum_height;
      fetch_rectangle.x =
        x + sampler->context_rect[0].x -
        (maximum_width  - sampler->context_rect[0].width ) / (gint) 4;
      fetch_rectangle.y =
        y + sampler->context_rect[0].y -
        (maximum_height - sampler->context_rect[0].height) / (gint) 4;

      gegl_buffer_get (sampler->buffer,
                       &fetch_rectangle,
                       1.0,
                       sampler->interpolate_format,
                       sampler->sampler_buffer[0],
                       GEGL_AUTO_ROWSTRIDE,
                       repeat_mode);

      sampler->sampler_rectangle[0] = fetch_rectangle;
    }

  dx         = x - sampler->sampler_rectangle[0].x;
  dy         = y - sampler->sampler_rectangle[0].y;
  buffer_ptr = (guchar *)sampler->sampler_buffer[0];
  sof        = ( dx + dy * sampler->sampler_rectangle[0].width ) * bpp;

  return (gfloat*)(buffer_ptr+sof);
}

gfloat *
gegl_sampler_get_from_buffer (GeglSampler *const sampler,
                              const gint         x,
                              const gint         y,
                              GeglAbyssPolicy    repeat_mode)
{
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gint bpp =
    babl_format_get_bytes_per_pixel (sampler->interpolate_format);

  const gint maximum_width  = GEGL_SAMPLER_MAXIMUM_WIDTH;
  const gint maximum_height = GEGL_SAMPLER_MAXIMUM_HEIGHT;
  g_assert (sampler->context_rect[0].width  <= maximum_width);
  g_assert (sampler->context_rect[0].height <= maximum_height);

  if ((sampler->sampler_buffer[0] == NULL)
      ||
      (x < sampler->sampler_rectangle[0].x)
      ||
      (y < sampler->sampler_rectangle[0].y)
      ||
      (x >=
       sampler->sampler_rectangle[0].x + sampler->sampler_rectangle[0].width)
      ||
      (y >=
       sampler->sampler_rectangle[0].y + sampler->sampler_rectangle[0].height))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle:
       */
      GeglRectangle fetch_rectangle;

      /*
       * Always request the same amount of pixels:
       */
      if (sampler->sampler_buffer[0] == NULL)
        {
          sampler->sampler_buffer[0] =
            g_malloc0 (maximum_width * maximum_height * bpp);
        }

      fetch_rectangle.width  = maximum_width;
      fetch_rectangle.height = maximum_height;
      fetch_rectangle.x = x -
        (maximum_width  - sampler->context_rect[0].width ) / (gint) 4;
      fetch_rectangle.y = y -
        (maximum_height - sampler->context_rect[0].height) / (gint) 4;

      gegl_buffer_get (sampler->buffer,
                       &fetch_rectangle,
                       1.0,
                       sampler->interpolate_format,
                       sampler->sampler_buffer[0],
                       GEGL_AUTO_ROWSTRIDE,
                       repeat_mode);

      sampler->sampler_rectangle[0] = fetch_rectangle;
    }

  dx         = x - sampler->sampler_rectangle[0].x;
  dy         = y - sampler->sampler_rectangle[0].y;
  buffer_ptr = (guchar *)sampler->sampler_buffer[0];
  sof        = ( dx + dy * sampler->sampler_rectangle[0].width ) * bpp;

  return (gfloat*)(buffer_ptr+sof);
}

gfloat *
gegl_sampler_get_from_mipmap (GeglSampler *const sampler,
                              const gint         x,
                              const gint         y,
                              const gint         level,
                              GeglAbyssPolicy    repeat_mode)
{
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gdouble scale = 1. / ( (gdouble) (1<<level) );

  const gint bpp =
    babl_format_get_bytes_per_pixel (sampler->interpolate_format);

  const gint maximum_width  = GEGL_SAMPLER_MAXIMUM_WIDTH;
  const gint maximum_height = GEGL_SAMPLER_MAXIMUM_HEIGHT;
  g_assert (level >= 0 && level < GEGL_SAMPLER_MIPMAP_LEVELS);
  g_assert (sampler->context_rect[level].width  <= maximum_width);
  g_assert (sampler->context_rect[level].height <= maximum_height);

  if ((sampler->sampler_buffer[level] == NULL)
      ||
      (x + sampler->context_rect[level].x < sampler->sampler_rectangle[level].x)
      ||
      (y + sampler->context_rect[level].y < sampler->sampler_rectangle[level].y)
      ||
      (x + sampler->context_rect[level].x +
       sampler->context_rect[level].width >
       sampler->sampler_rectangle[level].x +
       sampler->sampler_rectangle[level].width)
      ||
      (y + sampler->context_rect[level].y +
       sampler->context_rect[level].height >
       sampler->sampler_rectangle[level].y +
       sampler->sampler_rectangle[level].height))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle[level]:
       */
      GeglRectangle fetch_rectangle;

      /*
       * Always request the same amount of pixels:
       */
      if (sampler->sampler_buffer[level] == NULL)
        {
          sampler->sampler_buffer[level] =
            g_malloc0 (maximum_width * maximum_height * bpp);
        }

      fetch_rectangle.width  = maximum_width;
      fetch_rectangle.height = maximum_height;
      fetch_rectangle.x =
        x + sampler->context_rect[level].x -
        (maximum_width  - sampler->context_rect[level].width ) / (gint) 4;
      fetch_rectangle.y =
        y + sampler->context_rect[level].y -
        (maximum_height - sampler->context_rect[level].height) / (gint) 4;

      gegl_buffer_get (sampler->buffer,
                       &fetch_rectangle,
                       scale,
                       sampler->interpolate_format,
                       sampler->sampler_buffer[level],
                       GEGL_AUTO_ROWSTRIDE,
                       repeat_mode);

      sampler->sampler_rectangle[level] = fetch_rectangle;
    }

  dx         = x - sampler->sampler_rectangle[level].x;
  dy         = y - sampler->sampler_rectangle[level].y;
  buffer_ptr = (guchar *)sampler->sampler_buffer[level];
  sof        = ( dx + dy * sampler->sampler_rectangle[level].width ) * bpp;

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
        g_value_set_pointer (value, (void*)self->format);
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
        gegl_sampler_set_buffer (self, GEGL_BUFFER (g_value_get_object (value)));
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
      if (GEGL_IS_BUFFER (self->buffer))
        {
          g_signal_handlers_disconnect_by_func (self->buffer,
                                                G_CALLBACK (buffer_contents_changed),
                                                self);
          g_object_remove_weak_pointer ((GObject*) self->buffer, (void**) &self->buffer);
        }

      if (GEGL_IS_BUFFER (buffer))
        {
          self->buffer = buffer;
          g_object_add_weak_pointer ((GObject*) self->buffer, (void**) &self->buffer);
          g_signal_connect (buffer, "changed",
                            G_CALLBACK (buffer_contents_changed),
                            self);
        }
      else
        self->buffer = NULL;

      buffer_contents_changed (buffer, NULL, self);
    }
}

GeglSamplerType
gegl_sampler_type_from_string (const gchar *string)
{
  if (g_str_equal (string, "nearest") || g_str_equal (string, "none"))
    return GEGL_SAMPLER_NEAREST;

  if (g_str_equal (string, "linear")  || g_str_equal (string, "bilinear"))
    return GEGL_SAMPLER_LINEAR;

  if (g_str_equal (string, "cubic")   || g_str_equal (string, "bicubic"))
    return GEGL_SAMPLER_CUBIC;

  if (g_str_equal (string, "nohalo"))
    return GEGL_SAMPLER_NOHALO;

  if (g_str_equal (string, "lohalo"))
    return GEGL_SAMPLER_LOHALO;

  return GEGL_SAMPLER_LINEAR;
}

GType
gegl_sampler_gtype_from_enum (GeglSamplerType sampler_type)
{
  switch (sampler_type)
    {
      case GEGL_SAMPLER_NEAREST:
        return GEGL_TYPE_SAMPLER_NEAREST;
      case GEGL_SAMPLER_LINEAR:
        return GEGL_TYPE_SAMPLER_LINEAR;
      case GEGL_SAMPLER_CUBIC:
        return GEGL_TYPE_SAMPLER_CUBIC;
      case GEGL_SAMPLER_NOHALO:
        return GEGL_TYPE_SAMPLER_NOHALO;
      case GEGL_SAMPLER_LOHALO:
        return GEGL_TYPE_SAMPLER_LOHALO;
      default:
        return GEGL_TYPE_SAMPLER_LINEAR;
    }
}

GeglSampler *
gegl_buffer_sampler_new (GeglBuffer      *buffer,
                         const Babl      *format,
                         GeglSamplerType  sampler_type)
{
  GeglSampler *sampler;
  GType        desired_type;

  if (format == NULL)
    format = babl_format ("RaGaBaA float");

  desired_type = gegl_sampler_gtype_from_enum (sampler_type);

  sampler = g_object_new (desired_type,
                          "buffer", buffer,
                          "format", format,
                          NULL);

  gegl_sampler_prepare (sampler);

  return sampler;
}

const GeglRectangle*
gegl_sampler_get_context_rect (GeglSampler *sampler)
{
  return &(sampler->context_rect[0]);
}

static void
buffer_contents_changed (GeglBuffer          *buffer,
                         const GeglRectangle *changed_rect,
                         gpointer             userdata)
{
  GeglSampler *self = GEGL_SAMPLER (userdata);

  /*
   * Invalidate all mipmap levels by setting the width and height of the
   * rectangles to zero. The x and y coordinates do not matter any more, so we
   * can just call memset to do this.
   * XXX: it might be faster to only invalidate rects that intersect
   *      changed_rect
   */
  memset (self->sampler_rectangle, 0, sizeof (self->sampler_rectangle));

  return;
}
