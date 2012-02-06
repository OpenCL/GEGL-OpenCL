#ifndef __GEGL_BUFFER_CL_CACHE_H__
#define __GEGL_BUFFER_CL_CACHE_H__

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "opencl/gegl-cl.h"

cl_mem
gegl_buffer_cl_cache_get (GeglBuffer          *buffer,
                          const GeglRectangle *roi);

void
gegl_buffer_cl_cache_new (GeglBuffer            *buffer,
                          const GeglRectangle   *roi,
                          cl_mem                 tex);
gboolean
gegl_buffer_cl_cache_merge (GeglBuffer          *buffer,
                            const GeglRectangle *roi);

void
gegl_buffer_cl_cache_remove (GeglBuffer          *buffer,
                             const GeglRectangle *roi);

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi);

gboolean
gegl_buffer_cl_cache_from (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           gpointer             dest_buf,
                           const Babl          *format,
                           gint                 rowstride);
#endif
