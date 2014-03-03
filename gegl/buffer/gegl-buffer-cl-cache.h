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
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

#ifndef __GEGL_BUFFER_CL_CACHE_H__
#define __GEGL_BUFFER_CL_CACHE_H__

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"

#include "opencl/gegl-cl.h"

cl_mem
gegl_buffer_cl_cache_get (GeglBuffer          *buffer,
                          const GeglRectangle *roi);

gboolean
gegl_buffer_cl_cache_release (cl_mem tex);

void
gegl_buffer_cl_cache_new (GeglBuffer            *buffer,
                          const GeglRectangle   *roi,
                          cl_mem                 tex);

gboolean
gegl_buffer_cl_cache_flush  (GeglBuffer          *buffer,
                             const GeglRectangle *roi);

gboolean
gegl_buffer_cl_cache_flush2 (GeglTileHandlerCache *cache,
                             const GeglRectangle  *roi);

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi);

#endif
