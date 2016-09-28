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

#ifndef __GEGL_SAMPLER_H__
#define __GEGL_SAMPLER_H__

#include <glib-object.h>
#include <babl/babl.h>

G_BEGIN_DECLS

#define GEGL_TYPE_SAMPLER            (gegl_sampler_get_type ())
#define GEGL_SAMPLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLER, GeglSampler))
#define GEGL_SAMPLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLER, GeglSamplerClass))
#define GEGL_IS_SAMPLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLER))
#define GEGL_IS_SAMPLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLER))
#define GEGL_SAMPLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLER, GeglSamplerClass))

/*
 * This should be set to the largest number of mipmap levels (counted
 * starting at 0 = no box filtering) actually used by any sampler.
 */
#define GEGL_SAMPLER_MIPMAP_LEVELS (8)
/*
 * Best thing to do seems to use rectangular buffer tiles that are
 * twice as wide as they are tall.
 */

#define GEGL_SAMPLER_MAXIMUM_HEIGHT 64
#define GEGL_SAMPLER_MAXIMUM_WIDTH (GEGL_SAMPLER_MAXIMUM_HEIGHT)
#define GEGL_SAMPLER_BPP 16
#define GEGL_SAMPLER_ROWSTRIDE (GEGL_SAMPLER_MAXIMUM_WIDTH * GEGL_SAMPLER_BPP)

typedef struct _GeglSamplerClass GeglSamplerClass;

typedef struct GeglSamplerLevel
{
  GeglRectangle  context_rect;
  gpointer       sampler_buffer;
  GeglRectangle  sampler_rectangle;

  gint           last_x;
  gint           last_y;
  float          x_delta;
  float          y_delta;
  float          x_magnitude;
  float          y_magnitude;
} GeglSamplerLevel;

struct _GeglSampler
{
  GObject           parent_instance;
  GeglSamplerGetFun get;

  /*< private >*/
  GeglBuffer    *buffer;
  gint           lvel;
  const Babl    *format;
  const Babl    *interpolate_format;
  const Babl    *fish;

  GeglSamplerLevel level[GEGL_SAMPLER_MIPMAP_LEVELS];
};

struct _GeglSamplerClass
{
  GObjectClass  parent_class;

  void (* prepare)   (GeglSampler     *self);
  GeglSamplerGetFun   get;
  void  (*set_buffer) (GeglSampler     *self,
                       GeglBuffer      *buffer);
};

GType gegl_sampler_get_type    (void) G_GNUC_CONST;

/* virtual method invokers */
void  gegl_sampler_prepare             (GeglSampler *self);
void  gegl_sampler_set_buffer          (GeglSampler *self,
                                        GeglBuffer  *buffer);

gfloat * gegl_sampler_get_from_buffer (GeglSampler     *sampler,
                                       gint             x,
                                       gint             y,
                                       GeglAbyssPolicy  repeat_mode);
gfloat * gegl_sampler_get_from_mipmap (GeglSampler     *sampler,
                                       gint             x,
                                       gint             y,
                                       gint             level,
                                       GeglAbyssPolicy  repeat_mode);
gfloat * _gegl_sampler_get_ptr        (GeglSampler     *sampler,
                                       gint             x,
                                       gint             y,
                                       GeglAbyssPolicy  repeat_mode);

GeglRectangle _gegl_sampler_compute_rectangle (GeglSampler *sampler,
                                               gint         x,
                                               gint         y,
                                               gint         level);

/*
 * Gets a pointer to the center pixel, within a buffer that has a
 * rowstride of GEGL_SAMPLER_MAXIMUM_WIDTH * 16 (16 is the bpp of RaGaBaA
 * float).
 *
 * inlining this function gives a 4-5% performance gain for affine ops for
 * linear/cubic sampling.
 */
static inline gfloat *
gegl_sampler_get_ptr (GeglSampler    *sampler,
                      gint            x,
                      gint            y,
                      GeglAbyssPolicy repeat_mode)
{
  gint dx, dy, sof;
  guchar *buffer_ptr;

  GeglSamplerLevel *level = &sampler->level[0];
  if ((x + level->context_rect.x < level->sampler_rectangle.x)      ||
      (y + level->context_rect.y < level->sampler_rectangle.y)      ||
      (x + level->context_rect.x + level->context_rect.width >
       level->sampler_rectangle.x + level->sampler_rectangle.width) ||
      (y + level->context_rect.y + level->context_rect.height >
       level->sampler_rectangle.y + level->sampler_rectangle.height))
    {
      level->sampler_rectangle = _gegl_sampler_compute_rectangle (sampler, x, y, 0);

      gegl_buffer_get (sampler->buffer,
                       &level->sampler_rectangle,
                       1.0,
                       sampler->interpolate_format,
                       level->sampler_buffer,
                       GEGL_SAMPLER_MAXIMUM_WIDTH * GEGL_SAMPLER_BPP,
                       repeat_mode);
    }

  dx         = x - level->sampler_rectangle.x;
  dy         = y - level->sampler_rectangle.y;
  sof        = (dx + dy * GEGL_SAMPLER_MAXIMUM_WIDTH) * GEGL_SAMPLER_BPP;
  buffer_ptr = (guchar *) level->sampler_buffer;

  return (gfloat *) (buffer_ptr + sof);
}

G_END_DECLS

#endif /* __GEGL_SAMPLER_H__ */
