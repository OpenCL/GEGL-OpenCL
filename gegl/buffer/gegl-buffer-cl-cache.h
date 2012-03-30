#ifndef __GEGL_BUFFER_CL_CACHE_H__
#define __GEGL_BUFFER_CL_CACHE_H__

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
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
