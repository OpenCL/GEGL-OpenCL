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
#ifndef _GEGL_SAMPLER_NEAREST_H__
#define _GEGL_SAMPLER_NEAREST_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-sampler.h"
#include "buffer/gegl-buffer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_SAMPLER_NEAREST               (gegl_sampler_nearest_get_type ())
#define GEGL_SAMPLER_NEAREST(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLER_NEAREST, GeglSamplerNearest))
#define GEGL_SAMPLER_NEAREST_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLER_NEAREST, GeglSamplerNearestClass))
#define GEGL_SAMPLER_NEAREST_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLER_NEAREST, GeglSamplerNearestClass))

typedef struct _GeglSamplerNearest  GeglSamplerNearest;
struct _GeglSamplerNearest
{
    GeglSampler sampler;
    /*< private >*/
};

typedef struct _GeglSamplerNearestClass GeglSamplerNearestClass;
struct _GeglSamplerNearestClass
{
   GeglSamplerClass sampler_class;
};

GType          gegl_sampler_nearest_get_type  (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
