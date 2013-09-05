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
 * Copyright 2013 Carlos Zubieta (czubieta.dev@gmail.com)
 */

#ifndef __GEGL_CL_RANDOM_H__
#define __GEGL_CL_RANDOM_H__

#include "gegl-cl-types.h"
/** Load the random data needed to generate random numbers in the GPU*/
cl_mem gegl_cl_load_random_data(int *cl_err);

/** Load the primes needed to generate random numbers in the GPU*/
cl_mem gegl_cl_load_random_primes(int *cl_err);

/*
#define GEGL_CL_LOAD_RANDOM(obj)           \
   { obj = gegl_cl_load_random(&cl_err);   \
    CL_CHECK;}

#define GEGL_CL_RELEASE_RANDOM(obj)        \
  { cl_err = gegl_clReleaseMemObject(obj); \
    CL_CHECK; }
*/

#endif
