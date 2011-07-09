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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"

GType
gegl_sampler_type_get_type (void)
{
    static GType etype = 0;
    if (etype == 0) {
        static const GEnumValue values[] = {
            { GEGL_SAMPLER_NEAREST,   "GEGL_SAMPLER_NEAREST",   "nearest"   },
            { GEGL_SAMPLER_LINEAR,    "GEGL_SAMPLER_LINEAR",    "linear"    },
            { GEGL_SAMPLER_CUBIC,     "GEGL_SAMPLER_CUBIC",     "cubic"     },
            { GEGL_SAMPLER_LANCZOS,   "GEGL_SAMPLER_LANCZOS",   "lanczos"   },
            { GEGL_SAMPLER_LOHALO,    "GEGL_SAMPLER_LOHALO",    "lohalo"    },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static ("GeglSamplerTypeType", values);
    }
    return etype;
}
