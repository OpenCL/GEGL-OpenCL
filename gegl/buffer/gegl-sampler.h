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

/* this file needs to be included by gegl-buffer-private */

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
#define GEGL_SAMPLER_MAXIMUM_HEIGHT (64)
#define GEGL_SAMPLER_MAXIMUM_WIDTH (GEGL_SAMPLER_MAXIMUM_HEIGHT)

typedef struct _GeglSamplerClass GeglSamplerClass;

struct _GeglSampler
{
  GObject       parent_instance;
  void (* get) (GeglSampler     *self,
                gdouble          x,
                gdouble          y,
                GeglMatrix2     *scale,
                void            *output,
                GeglAbyssPolicy  repeat_mode);
  /* we cache the getter in the instance, (being able to return the
     function pointer itself and cache it outside the calling loop
     would be even quicker.
   */

  /*< private >*/
  GeglBuffer    *buffer;
  const Babl    *format;
  const Babl    *interpolate_format;
  const Babl    *fish;
  GeglRectangle  context_rect[GEGL_SAMPLER_MIPMAP_LEVELS];
  gpointer       sampler_buffer[GEGL_SAMPLER_MIPMAP_LEVELS];
  GeglRectangle  sampler_rectangle[GEGL_SAMPLER_MIPMAP_LEVELS];
  gdouble        x; /* mirrors the currently requested */
  gdouble        y; /* coordinates in the instance     */

  gpointer       padding[8]; /* eat from the padding if adding to the struct */
};

struct _GeglSamplerClass
{
  GObjectClass  parent_class;

  void (* prepare)   (GeglSampler     *self);
  void (* get)       (GeglSampler     *self,
                      gdouble          x,
                      gdouble          y,
                      GeglMatrix2     *scale,
                      void            *output,
                      GeglAbyssPolicy  repeat_mode);
 void  (*set_buffer) (GeglSampler     *self,
                      GeglBuffer      *buffer);

 gpointer       padding[8]; /* eat from the padding if adding to the struct */
};

GType gegl_sampler_get_type    (void) G_GNUC_CONST;

/* virtual method invokers */
void  gegl_sampler_prepare     (GeglSampler *self);
void  gegl_sampler_set_buffer  (GeglSampler *self,
                                GeglBuffer  *buffer);

void  gegl_sampler_get         (GeglSampler   *self,
                                gdouble        x,
                                gdouble        y,
                                GeglMatrix2   *scale,
                                void          *output,
                                GeglAbyssPolicy repeat_mode);

gfloat * gegl_sampler_get_from_buffer (GeglSampler *const sampler,
                                       const gint         x,
                                       const gint         y,
                                       GeglAbyssPolicy    repeat_mode);
gfloat * gegl_sampler_get_from_mipmap (GeglSampler *const sampler,
                                       const gint         x,
                                       const gint         y,
                                       const gint         level,
                                       GeglAbyssPolicy    repeat_mode);
gfloat * gegl_sampler_get_ptr         (GeglSampler *const sampler,
                                       const gint         x,
                                       const gint         y,
                                       GeglAbyssPolicy    repeat_mode);

G_END_DECLS

#endif /* __GEGL_SAMPLER_H__ */
