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
#ifndef _GEGL_INTERPOLATOR_H__
#define _GEGL_INTERPOLATOR_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "buffer/gegl-buffer-types.h"
#include <babl/babl.h>

G_BEGIN_DECLS

#define GEGL_TYPE_INTERPOLATOR               (gegl_interpolator_get_type ())
#define GEGL_INTERPOLATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_INTERPOLATOR, GeglInterpolator))
#define GEGL_INTERPOLATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_INTERPOLATOR, GeglInterpolatorClass))
#define GEGL_IS_INTERPOLATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_INTERPOLATOR))
#define GEGL_IS_INTERPOLATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_INTERPOLATOR))
#define GEGL_INTERPOLATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_INTERPOLATOR, GeglInterpolatorClass))

typedef struct _GeglInterpolatorClass GeglInterpolatorClass;
  
struct _GeglInterpolator
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

struct _GeglInterpolatorClass
{
  GObjectClass  parent_class;
  void (*prepare) (GeglInterpolator *self);                    
  void (*get)     (GeglInterpolator *self,
                   gdouble           x,
                   gdouble           y,
                   void             *output);
};

/* virtual method invokers */
void  gegl_interpolator_prepare               (GeglInterpolator *self);
void  gegl_interpolator_get                   (GeglInterpolator *self,
                                               gdouble           x,
                                               gdouble           y,
                                               void             *output);
GType gegl_interpolator_get_type              (void) G_GNUC_CONST;

void  gegl_interpolator_fill_buffer           (GeglInterpolator *interpolator,
                                               gdouble           x,
                                               gdouble           y);

G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
