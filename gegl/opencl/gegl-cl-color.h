#ifndef __GEGL_CL_COLOR_H__
#define __GEGL_CL_COLOR_H__

#include <gegl.h>
#include "gegl-cl-types.h"

typedef enum
{
  GEGL_CL_COLOR_NOT_SUPPORTED = 0,
  GEGL_CL_COLOR_EQUAL         = 1,
  GEGL_CL_COLOR_CONVERT       = 2
} gegl_cl_color_op;

void gegl_cl_color_compile_kernels(void);

gboolean gegl_cl_color_babl (const Babl *buffer_format, size_t *bytes);

gegl_cl_color_op gegl_cl_color_supported (const Babl *in_format, const Babl *out_format);

gboolean gegl_cl_color_conv (cl_mem in_tex, cl_mem aux_tex, const size_t size,
                             const Babl *in_format, const Babl *out_format);

#endif
