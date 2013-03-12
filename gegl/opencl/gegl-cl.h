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

#ifndef __GEGL_CL_H__
#define __GEGL_CL_H__

#include "gegl-cl-types.h"
#include "gegl-cl-init.h"
#include "gegl-cl-color.h"

#ifdef __GEGL_DEBUG_H__

#define CL_ERROR {GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

#else

/* public header for gegl ops */

#define CL_ERROR {g_warning("Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

#endif

#define CL_CHECK {if (cl_err != CL_SUCCESS) CL_ERROR;}

#define GEGL_CL_ARG_START(KERNEL) \
  { cl_kernel __mykernel=KERNEL; int __p = 0;

#define GEGL_CL_ARG(TYPE, NAME) \
  { cl_err = gegl_clSetKernelArg(__mykernel, __p++, sizeof(TYPE), (void*)& NAME); \
    CL_CHECK; }

#define GEGL_CL_ARG_END \
  __p = -1; }

#define GEGL_CL_RELEASE(obj)               \
  { cl_err = gegl_clReleaseMemObject(obj); \
    CL_CHECK; }

#define GEGL_CL_BUFFER_ITERATE_START(I, J, ERR)      \
  while (gegl_buffer_cl_iterator_next (I, & ERR)) \
    {                                                \
      if (ERR) return FALSE;                         \
      for (J=0; J < I ->n; J++)                      \
        {

#define GEGL_CL_BUFFER_ITERATE_END(ERR)   \
          if (ERR)                        \
           {                              \
             g_warning("[OpenCL] Error"); \
             return FALSE;                \
           }                              \
        }                                 \
    }


#define GEGL_CL_BUILD(NAME, ...)                                            \
  if (!cl_data)                                                             \
    {                                                                       \
      const char *kernel_name[] ={__VA_ARGS__ , NULL};                      \
      cl_data = gegl_cl_compile_and_build(NAME ## _cl_source, kernel_name); \
    }                                                                       \
  if (!cl_data) return TRUE;

#define GEGL_CL_STATIC \
  static GeglClRunData *cl_data = NULL;

#endif
