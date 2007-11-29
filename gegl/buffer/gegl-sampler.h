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
#ifndef _GEGL_SAMPLER_H__
#define _GEGL_SAMPLER_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "buffer/gegl-buffer-types.h"
#include <babl/babl.h>

G_BEGIN_DECLS

#define GEGL_TYPE_SAMPLER               (gegl_sampler_get_type ())
#define GEGL_SAMPLER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLER, GeglSampler))
#define GEGL_SAMPLER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLER, GeglSamplerClass))
#define GEGL_IS_SAMPLER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLER))
#define GEGL_IS_SAMPLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLER))
#define GEGL_SAMPLER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLER, GeglSamplerClass))

typedef struct _GeglSamplerClass GeglSamplerClass;
  
struct _GeglSampler
{
  GObject parent_instance;
  /*< private >*/
  GeglBuffer    *buffer;
  Babl          *format;

  GeglRectangle  cache_rectangle;
  void          *cache_buffer;
  Babl          *interpolate_format;
  gint           context_pixels;
};

struct _GeglSamplerClass
{
  GObjectClass  parent_class;
  void (*prepare) (GeglSampler *self);                    
  void (*get)     (GeglSampler *self,
                   gdouble           x,
                   gdouble           y,
                   void             *output);
};

/* virtual method invokers */
void  gegl_sampler_prepare               (GeglSampler *self);
void  gegl_sampler_get                   (GeglSampler *self,
                                          gdouble           x,
                                          gdouble           y,
                                          void             *output);
GType gegl_sampler_get_type              (void) G_GNUC_CONST;

void  gegl_sampler_fill_buffer           (GeglSampler *sampler,
                                          gdouble           x,
                                          gdouble           y);

G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
