#ifndef __GEGL_CL_TYPES_H__
#define __GEGL_CL_TYPES_H__

#include <glib-object.h>

#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_ext.h>

typedef cl_int (*t_clGetPlatformIDs)(cl_uint,
                                     cl_platform_id *,
                                     cl_uint *);

typedef cl_int (*t_clGetPlatformInfo)(cl_platform_id,
                                      cl_platform_info,
                                      size_t,
                                      void *,
                                      size_t *);

#endif /* __GEGL_CL_TYPES_H__ */
