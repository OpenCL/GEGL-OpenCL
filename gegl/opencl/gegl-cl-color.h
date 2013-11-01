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

#ifndef __GEGL_CL_COLOR_H__
#define __GEGL_CL_COLOR_H__

#include <gegl.h>
#include "gegl-cl-types.h"

typedef enum
{
  GEGL_CL_COLOR_NOT_SUPPORTED = 0,
  GEGL_CL_COLOR_EQUAL         = 1,
  GEGL_CL_COLOR_CONVERT       = 2
} GeglClColorOp;

/** Compile and register OpenCL kernel for color conversion */
gboolean      gegl_cl_color_compile_kernels (void);

/** Return TRUE if the Babl format is supported with OpenCL.
 *  If present, returns the byte per pixel in *bytes
 */
gboolean      gegl_cl_color_babl (const Babl *buffer_format,
                                  size_t     *bytes);

/** Return TRUE if the convertion is OpenCL supported */
GeglClColorOp gegl_cl_color_supported (const Babl *in_format,
                                       const Babl *out_format);

/** Copy and convert size pixels from in_tex to aux_tex.
 *  Return TRUE if succesfull.
 */
gboolean      gegl_cl_color_conv (cl_mem        in_tex,
                                  cl_mem        aux_tex,
                                  const size_t  size,
                                  const Babl   *in_format,
                                  const Babl   *out_format);

#endif
