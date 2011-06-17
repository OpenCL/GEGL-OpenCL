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

typedef struct _GeglSamplerClass GeglSamplerClass;
typedef struct _GeglSampler    GeglSampler;

struct _GeglSampler
{
  GObject        parent_instance;

  /*< private >*/
  GeglBuffer    *buffer;
  Babl          *format;
  Babl          *interpolate_format;
  Babl          *fish;
  GeglRectangle  context_rect;
  void          *sampler_buffer;
  GeglRectangle  sampler_rectangle;
  GeglMatrix2   *inverse_jacobian;
  gdouble        x; /* mirrors the currently requested */
  gdouble        y; /* coordinates in the instance     */
};

struct _GeglSamplerClass
{
  GObjectClass  parent_class;

  void (* prepare)   (GeglSampler *self);
  void (* get)       (GeglSampler *self,
                      gdouble      x,
                      gdouble      y,
                      void        *output);
 void  (*set_buffer) (GeglSampler  *self,
                      GeglBuffer   *buffer);
};

GType gegl_sampler_get_type    (void) G_GNUC_CONST;

/* virtual method invokers */
void  gegl_sampler_prepare     (GeglSampler *self);
void  gegl_sampler_set_buffer  (GeglSampler *self,
                                GeglBuffer  *buffer);

void  gegl_sampler_get         (GeglSampler *self,
                                gdouble      x,
                                gdouble      y,
                                void        *output);
gfloat * gegl_sampler_get_from_buffer (GeglSampler *sampler,
                                       gint         x,
                                       gint         y);

gfloat *
gegl_sampler_get_ptr (GeglSampler         *sampler,
                      gint                 x,
                      gint                 y);
GType gegl_sampler_type_from_interpolation (GeglInterpolation interpolation);

G_END_DECLS

#endif /* __GEGL_SAMPLER_H__ */
