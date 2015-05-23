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
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-buffer-cl-cache.h"

#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-nohalo.h"
#include "gegl-sampler-lohalo.h"

#include "gegl-config.h"

enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_FORMAT,
  PROP_LEVEL,
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

static void constructed (GObject *sampler);

static GType gegl_sampler_gtype_from_enum  (GeglSamplerType      sampler_type);

G_DEFINE_TYPE (GeglSampler, gegl_sampler, G_TYPE_OBJECT)

static void
gegl_sampler_class_init (GeglSamplerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;
  object_class->dispose  = dispose;
  object_class->constructed  = constructed;

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
                 PROP_LEVEL,
                 g_param_spec_int ("level",
                                   "level",
                                   "mimmap level to sample from",
                                   0, 100, 0,
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
gegl_sampler_init (GeglSampler *sampler)
{
  gint i = 0;
  sampler->buffer = NULL;
  do {
    GeglRectangle context_rect      = {0,0,1,1};
    GeglRectangle sampler_rectangle = {0,0,0,0};
    sampler->level[i].sampler_buffer = NULL;
    sampler->level[i].context_rect   = context_rect;
    sampler->level[i].sampler_rectangle = sampler_rectangle;
  } while ( ++i<GEGL_SAMPLER_MIPMAP_LEVELS );

  sampler->level[0].sampler_buffer =
    g_malloc0 (GEGL_SAMPLER_MAXIMUM_WIDTH *
               GEGL_SAMPLER_MAXIMUM_HEIGHT * GEGL_SAMPLER_BPP);
}

static void
constructed (GObject *self)
{
  GeglSampler *sampler = (void*)(self);
  GeglSamplerClass *klass = GEGL_SAMPLER_GET_CLASS (sampler);
  sampler->get = klass->get;
}

void
gegl_sampler_get (GeglSampler     *self,
                  gdouble          x,
                  gdouble          y,
                  GeglMatrix2     *scale,
                  void            *output,
                  GeglAbyssPolicy  repeat_mode)
{
  if (self->lvel)
  {
    double factor = 1.0 / (1 << self->lvel);
    GeglRectangle rect={floorf (x * factor), floorf (y * factor),1,1};
    gegl_buffer_get (self->buffer, &rect, factor, self->format, output, GEGL_AUTO_ROWSTRIDE, repeat_mode);
    return;
  }

  if (gegl_cl_is_accelerated ())
    {
      GeglRectangle rect={x,y,1,1};
      gegl_buffer_cl_cache_flush (self->buffer, &rect);
    }
  self->get (self, x, y, scale, output, repeat_mode);
}

void
gegl_sampler_prepare (GeglSampler *self)
{
  GeglSamplerClass *klass;

  g_return_if_fail (GEGL_IS_SAMPLER (self));

  klass = GEGL_SAMPLER_GET_CLASS (self);

  if (!self->buffer) /* happens when extent of sampler is queried */
    return;
  if (!self->format)
    self->format = self->buffer->soft_format;

  if (klass->prepare)
    klass->prepare (self);
  
  if (!self->fish)
    self->fish = babl_fish (self->interpolate_format, self->format);

  /*
   * This makes the cache rect invalid, in case the data in the buffer
   * has changed:
   */
  self->level[0].sampler_rectangle.width = 0;
  self->level[0].sampler_rectangle.height = 0;

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
    if (sampler->level[i].sampler_buffer)
      {
        g_free (sampler->level[i].sampler_buffer);
        sampler->level[i].sampler_buffer = NULL;
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

GeglRectangle _gegl_sampler_compute_rectangle (GeglSampler *sampler,
                                               gint         x,
                                               gint         y,
                                               gint         level_no)
{
  GeglRectangle rectangle;
  GeglSamplerLevel *level = &sampler->level[level_no];

  if (level->last_x || level->last_y)
  {
    gint x_delta = x - level->last_x;
    gint y_delta = y - level->last_y;
    gint max_delta_squared = 60 * 60;

    if (x_delta * x_delta < max_delta_squared &&
        y_delta * y_delta < max_delta_squared)
    {
      level->x_delta = level->x_delta * 0.99 + x_delta * 0.01;
      level->y_delta = level->y_delta * 0.99 + y_delta * 0.01;
      if (x_delta < 0) x_delta = -x_delta;
      if (y_delta < 0) y_delta = -y_delta;
      level->x_magnitude = level->x_magnitude  * 0.99 + x_delta * 0.01;
      level->y_magnitude = level->y_magnitude  * 0.99 + y_delta * 0.01;
    }
  }
  level->last_x = x;
  level->last_y = y;

  rectangle.width  = level->context_rect.width + 2;
  rectangle.height = level->context_rect.height + 2;

  rectangle.x = x + level->context_rect.x;
  rectangle.y = y + level->context_rect.y;

  /* grow area slightly, maybe getting a couple of extra
     hits out of cour cache
   */
  rectangle.x      -= 2;
  rectangle.y      -= 2;
  rectangle.width  += 4;
  rectangle.height += 4;

#if 0
  /* XXX: FIXME: grow cached area based on sampling pattern profiling
   * information, possibly with much bigger wins than the marginal wins the
   * increase in size by 2 in each dir does.
   */


  if (level->x_magnitude > 0.001 || level->y_magnitude > 0.001)
    {
      gfloat magnitude = sqrtf (level->x_magnitude * level->x_magnitude +
                                level->y_magnitude * level->y_magnitude);
      gfloat new_magnitude;

      if (level->x_magnitude > level->y_magnitude)
        new_magnitude =
          4 + 60 * (level->x_magnitude - level->y_magnitude) / level->x_magnitude;
      else
        new_magnitude =
          4 + 60 * (level->y_magnitude - level->x_magnitude) / level->y_magnitude;

      rectangle.width = 
        (level->x_magnitude / magnitude) * new_magnitude
        + level->context_rect.width + 2;
      rectangle.height = 
        (level->y_magnitude / magnitude) * new_magnitude
        + level->context_rect.height + 2;

      /* todo: if both xmag and ymag are small - but similar in magnitude 
         we should increase the size of the cache if it would fit, thus
         perhaps working better on small local non-linear access patterns
       */


      /* align rectangle corner we've likely entered with sampled pixel
       */
      if (level->x_delta >=0)
        rectangle.x = x + level->context_rect.x
                        - (rectangle.width - level->context_rect.x)/10;
      else
        rectangle.x = x + level->context_rect.x
                        - (rectangle.width - level->context_rect.width) * 9/10;

      if (level->y_delta >=0)
        rectangle.y = y + level->context_rect.y
                        - (rectangle.height - level->context_rect.y)/10;
      else
        rectangle.y = y + level->context_rect.y 
                        - (rectangle.height - level->context_rect.height) * 9/10;
    }
#endif

  if (rectangle.width >= GEGL_SAMPLER_MAXIMUM_WIDTH)
    rectangle.width = GEGL_SAMPLER_MAXIMUM_WIDTH;
  if (rectangle.height >= GEGL_SAMPLER_MAXIMUM_HEIGHT)
    rectangle.height = GEGL_SAMPLER_MAXIMUM_HEIGHT;

  if (rectangle.width < level->context_rect.width)
    rectangle.width = level->context_rect.width;
  if (rectangle.height < level->context_rect.height)
    rectangle.height = level->context_rect.height;

  return rectangle;
}


gfloat *
gegl_sampler_get_from_mipmap (GeglSampler    *sampler,
                              gint            x,
                              gint            y,
                              gint            level_no,
                              GeglAbyssPolicy repeat_mode)
{
  GeglSamplerLevel *level = &sampler->level[level_no];
  guchar *buffer_ptr;
  gint    dx;
  gint    dy;
  gint    sof;

  const gdouble scale = 1. / ( (gdouble) (1<<level_no) );

  const gint maximum_width  = GEGL_SAMPLER_MAXIMUM_WIDTH;
  const gint maximum_height = GEGL_SAMPLER_MAXIMUM_HEIGHT;

  if (gegl_cl_is_accelerated ())
    {
      GeglRectangle rect={x,y,1,1};
      gegl_buffer_cl_cache_flush (sampler->buffer, &rect);
    }

  g_assert (level_no >= 0 && level_no < GEGL_SAMPLER_MIPMAP_LEVELS);
  g_assert (level->context_rect.width  <= maximum_width);
  g_assert (level->context_rect.height <= maximum_height);


  if ((level->sampler_buffer == NULL)
   || (x + level->context_rect.x < level->sampler_rectangle.x)
   || (y + level->context_rect.y < level->sampler_rectangle.y)
   || (x + level->context_rect.x + level->context_rect.width
     > level->sampler_rectangle.x + level->sampler_rectangle.width)
   || (y + level->context_rect.y + level->context_rect.height
     > level->sampler_rectangle.y + level->sampler_rectangle.height))
    {
      /*
       * fetch_rectangle will become the value of
       * sampler->sampler_rectangle[level]:
       */
      level->sampler_rectangle = _gegl_sampler_compute_rectangle (sampler, x, y, 
                                                                  level_no);
      if (!level->sampler_buffer)
        level->sampler_buffer =
          g_malloc0 (GEGL_SAMPLER_ROWSTRIDE * GEGL_SAMPLER_MAXIMUM_HEIGHT);


      gegl_buffer_get (sampler->buffer,
                       &level->sampler_rectangle,
                       scale,
                       sampler->interpolate_format,
                       level->sampler_buffer,
                       GEGL_SAMPLER_ROWSTRIDE,
                       repeat_mode);
    }

  dx         = x - level->sampler_rectangle.x;
  dy         = y - level->sampler_rectangle.y;
  buffer_ptr = (guchar *)level->sampler_buffer;
  sof        = ( dx + dy * GEGL_SAMPLER_MAXIMUM_WIDTH) * GEGL_SAMPLER_BPP;

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

      case PROP_LEVEL:
        g_value_set_int (value, self->lvel);
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

      case PROP_LEVEL:
        self->lvel = g_value_get_int  (value);
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
          self->buffer->changed_signal_connections--;
          g_object_remove_weak_pointer ((GObject*) self->buffer, (void**) &self->buffer);
        }

      if (GEGL_IS_BUFFER (buffer))
        {
          self->buffer = buffer;
          g_object_add_weak_pointer ((GObject*) self->buffer, (void**) &self->buffer);
          gegl_buffer_signal_connect (buffer, "changed",
                                      G_CALLBACK (buffer_contents_changed),
                                      self);
        }
      else
        self->buffer = NULL;

      buffer_contents_changed (buffer, NULL, self);
    }
}

static inline GType
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

void
gegl_buffer_sample_at_level (GeglBuffer       *buffer,
                             gdouble           x,
                             gdouble           y,
                             GeglMatrix2      *scale,
                             gpointer          dest,
                             const Babl       *format,
                             gint              level,
                             GeglSamplerType   sampler_type,
                             GeglAbyssPolicy   repeat_mode)
{
  GType desired_type;
  /*
  if (sampler_type == GEGL_SAMPLER_NEAREST && format == buffer->soft_format)
  {
    GeglRectangle rect = {floorf (x), floorf(y), 1, 1};
    gegl_buffer_get (buffer, &rect, 1, NULL, dest, GEGL_AUTO_ROWSTRIDE, repeat_mode);
    return;
  }*/

  static GMutex mutex = {0,};
  gboolean threaded =  gegl_config_threads ()>1;


  if (!format)
    format = buffer->soft_format;

  if (gegl_cl_is_accelerated ())
  {
    GeglRectangle rect = {floorf (x), floorf(y), 1, 1};
    gegl_buffer_cl_cache_flush (buffer, &rect);
  }

  if (threaded)
    g_mutex_lock (&mutex);

  /* unset the cached sampler if it dosn't match the needs */
  if (buffer->sampler != NULL &&
     (buffer->sampler_type != sampler_type ||
       buffer->sampler_format != format
      ))
    {
      g_object_unref (buffer->sampler);
      buffer->sampler = NULL;
      buffer->sampler_type = 0;
    }

  /* look up appropriate sampler,. */
  if (buffer->sampler == NULL)
    {
      desired_type = gegl_sampler_gtype_from_enum (sampler_type);
      buffer->sampler_type = sampler_type;
      buffer->sampler = g_object_new (desired_type,
                                      "buffer", buffer,
                                      "format", format,
                                      "level", level,
                                      NULL);
      buffer->sampler_format = format;
      gegl_sampler_prepare (buffer->sampler);
    }

  buffer->sampler->get(buffer->sampler, x, y, scale, dest, repeat_mode);
  if (threaded)
    g_mutex_unlock (&mutex);
}


void
gegl_buffer_sample (GeglBuffer       *buffer,
                    gdouble           x,
                    gdouble           y,
                    GeglMatrix2      *scale,
                    gpointer          dest,
                    const Babl       *format,
                    GeglSamplerType   sampler_type,
                    GeglAbyssPolicy   repeat_mode)
{
  gegl_buffer_sample_at_level (buffer, x, y, scale, dest, format, 0, sampler_type, repeat_mode);
}

GeglSampler *
gegl_buffer_sampler_new_at_level (GeglBuffer      *buffer,
                                  const Babl      *format,
                                  GeglSamplerType  sampler_type,
                                  gint             level)
{
  GeglSampler *sampler;
  GType        desired_type;

  if (format == NULL)
    format = babl_format ("RaGaBaA float");

  desired_type = gegl_sampler_gtype_from_enum (sampler_type);

  sampler = g_object_new (desired_type,
                          "buffer", buffer,
                          "format", format,
                          "level", level,
                          NULL);

  gegl_sampler_prepare (sampler);

  return sampler;
}

GeglSampler *
gegl_buffer_sampler_new (GeglBuffer      *buffer,
                         const Babl      *format,
                         GeglSamplerType  sampler_type)
{
  return gegl_buffer_sampler_new_at_level (buffer, format, sampler_type, 0);
}


const GeglRectangle*
gegl_sampler_get_context_rect (GeglSampler *sampler)
{
  return &(sampler->level[0].context_rect);
}

static void
buffer_contents_changed (GeglBuffer          *buffer,
                         const GeglRectangle *changed_rect,
                         gpointer             userdata)
{
  GeglSampler *self = GEGL_SAMPLER (userdata);
  int i;

  /*
   * Invalidate all mipmap levels by setting the width and height of the
   * rectangles to zero. The x and y coordinates do not matter any more, so we
   * can just call memset to do this.
   * XXX: it might be faster to only invalidate rects that intersect
   *      changed_rect
   */
  for (i = 0; i < GEGL_SAMPLER_MIPMAP_LEVELS; i++)
    memset (&self->level[i].sampler_rectangle, 0, sizeof (self->level[0].sampler_rectangle));

  return;
}

GeglSamplerGetFun gegl_sampler_get_fun (GeglSampler *sampler)
{
  /* this flushes the buffer in preparation for the use of the sampler,
   * thus one can consider the handed out sampler function only temporarily
   * available*/
  if (gegl_cl_is_accelerated ())
    gegl_buffer_cl_cache_flush (sampler->buffer, NULL);
  return sampler->get;
}

