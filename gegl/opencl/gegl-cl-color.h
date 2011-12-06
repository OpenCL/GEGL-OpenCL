#ifndef __GEGL_CL_COLOR_H__
#define __GEGL_CL_COLOR_H__

#include <gegl.h>
#include "gegl-cl-types.h"

void gegl_cl_color_compile_kernels(void);

gboolean gegl_cl_color_supported (const Babl *in_format, const Babl *out_format);

gboolean gegl_cl_color_conv (cl_mem in_tex, cl_mem out_tex, const size_t size[2],
                             const Babl *in_format, const Babl *out_format);

#endif
