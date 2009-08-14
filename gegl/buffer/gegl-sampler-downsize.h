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
#ifndef __GEGL_SAMPLER_DOWNSIZE_H__
#define __GEGL_SAMPLER_DOWNSIZE_H__

#include "gegl-sampler.h"

G_BEGIN_DECLS

#define GEGL_TYPE_SAMPLER_DOWNSIZE            (gegl_sampler_downsize_get_type ())
#define GEGL_SAMPLER_DOWNSIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SAMPLER_DOWNSIZE, GeglSamplerDownsize))
#define GEGL_SAMPLER_DOWNSIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SAMPLER_DOWNSIZE, GeglSamplerDownsizeClass))
#define GEGL_IS_SAMPLER_DOWNSIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SAMPLER_DOWNSIZE))
#define GEGL_IS_SAMPLER_DOWNSIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SAMPLER_DOWNSIZE))
#define GEGL_SAMPLER_DOWNSIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SAMPLER_DOWNSIZE, GeglSamplerDownsizeClass))

typedef struct _GeglSamplerDownsize      GeglSamplerDownsize;
typedef struct _GeglSamplerDownsizeClass GeglSamplerDownsizeClass;

struct _GeglSamplerDownsize
{
  GeglSampler parent_instance;
};

struct _GeglSamplerDownsizeClass
{
  GeglSamplerClass parent_class;
};

GType gegl_sampler_downsize_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
