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
#ifndef _GEGL_INTERPOLATOR_LANCZOS_H__
#define _GEGL_INTERPOLATOR_LANCZOS_H__

//#include <glib-object.h>
//#include "gegl-types.h"
#include "gegl-interpolator.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_INTERPOLATOR_LANCZOS               (gegl_interpolator_lanczos_get_type ())
#define GEGL_INTERPOLATOR_LANCZOS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_INTERPOLATOR_LANCZOS, GeglInterpolatorLanczos))
#define GEGL_INTERPOLATOR_LANCZOS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_INTERPOLATOR_LANCZOS, GeglInterpolatorLanczosClass))
#define GEGL_INTERPOLATOR_LANCZOS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_INTERPOLATOR_LANCZOS, GeglInterpolatorLanczosClass))

typedef struct _GeglInterpolatorLanczos  GeglInterpolatorLanczos;
struct _GeglInterpolatorLanczos
{
    GeglInterpolator interpolator;
    /*< private >*/
    gfloat *lanczos_lookup;
    gint    lanczos_width;
    gint    lanczos_spp;
};

typedef struct _GeglInterpolatorLanczosClass GeglInterpolatorLanczosClass;
struct _GeglInterpolatorLanczosClass
{
   GeglInterpolatorClass interpolator_class;
   void (*prepare) (GeglInterpolator *self);
   void (*get)     (GeglInterpolator *self,
                    gdouble           x,
                    gdouble           y,
                    void             *output);
};

GType                   gegl_interpolator_lanczos_get_type  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
