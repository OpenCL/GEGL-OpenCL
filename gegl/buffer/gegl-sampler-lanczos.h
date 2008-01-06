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

#ifndef __GEGL_SAMPLER_LANCZOS_H__
#define __GEGL_SAMPLER_LANCZOS_H__

#include "gegl-sampler.h"

G_BEGIN_DECLS

#define GEGL_TYPE_SAMPLER_LANCZOS             (gegl_sampler_lanczos_get_type ())
#define GEGL_SAMPLER_LANCZOS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLER_LANCZOS, GeglSamplerLanczos))
#define GEGL_SAMPLER_LANCZOS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLER_LANCZOS, GeglSamplerLanczosClass))
#define GEGL_IS_SAMPLER_LANCZOS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLER_LANCZOS))
#define GEGL_IS_SAMPLER_LANCZOS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),   GEGL_TYPE_SAMPLER_LANCZOS))
#define GEGL_SAMPLER_LANCZOS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),   GEGL_TYPE_SAMPLER_LANCZOS, GeglSamplerLanczosClass))

typedef struct _GeglSamplerLanczos      GeglSamplerLanczos;
typedef struct _GeglSamplerLanczosClass GeglSamplerLanczosClass;

struct _GeglSamplerLanczos
{
  GeglSampler  parent_instance;

  /*< private >*/
  gfloat      *lanczos_lookup;
  gint         lanczos_width;
  gint         lanczos_spp;
};

struct _GeglSamplerLanczosClass
{
  GeglSamplerClass  parent_class;
};

GType gegl_sampler_lanczos_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
