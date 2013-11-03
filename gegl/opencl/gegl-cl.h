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
#include "gegl-cl-random.h"

cl_int gegl_cl_set_kernel_args (cl_kernel kernel, ...) G_GNUC_NULL_TERMINATED;

#define CL_ERROR \
{                                          \
  g_warning ("Error in %s:%d@%s - %s\n",   \
             __FILE__, __LINE__, __func__, \
             gegl_cl_errstring (cl_err));  \
  goto error;                              \
}

#define CL_CHECK { if (cl_err != CL_SUCCESS) CL_ERROR; }

#define CL_CHECK_ONLY(errcode) \
if (errcode != CL_SUCCESS)                  \
{                                           \
  g_warning ("Error in %s:%d@%s - %s\n",    \
             __FILE__, __LINE__, __func__,  \
             gegl_cl_errstring (errcode));  \
}

#endif
