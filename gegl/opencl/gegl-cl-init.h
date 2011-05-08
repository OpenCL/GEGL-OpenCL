#ifndef __GEGL_CL_INIT_H__
#define __GEGL_CL_INIT_H__

#include "gegl-cl-types.h"
#include <gmodule.h>

gboolean gegl_cl_init (GError **error);

gboolean gegl_cl_is_accelerated (void);

#ifdef __GEGL_CL_INIT_MAIN__
t_clGetPlatformIDs  gegl_clGetPlatformIDs  = NULL;
t_clGetPlatformInfo gegl_clGetPlatformInfo = NULL;
#else
extern t_clGetPlatformIDs  gegl_clGetPlatformIDs;
extern t_clGetPlatformInfo gegl_clGetPlatformInfo;
#endif

#endif  /* __GEGL_CL_INIT_H__ */
