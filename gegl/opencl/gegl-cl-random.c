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

#include <glib.h>
#include "gegl-cl-random.h"
#include "opencl/gegl-cl.h"

/*XXX: defined in gegl-random.c*/
#define RANDOM_DATA_SIZE (15083+15091+15101)
#define PRIMES_SIZE 533

extern gint32      *gegl_random_data;
extern long        *gegl_random_primes;
extern inline void gegl_random_init (void);

cl_mem gegl_cl_load_random_data(int *cl_err)
{
  gegl_random_init();
  cl_mem cl_random_data;
  cl_random_data = gegl_clCreateBuffer(gegl_cl_get_context(),
                                       CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                       RANDOM_DATA_SIZE*sizeof(gint32),
                                       (void*) &gegl_random_data,
                                       cl_err);
  return cl_random_data;
}

cl_mem gegl_cl_load_random_primes(int *cl_err)
{
  cl_mem cl_random_primes;
  cl_random_primes = gegl_clCreateBuffer(gegl_cl_get_context(),
                                         CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                         PRIMES_SIZE*sizeof(long),
                                         (void*) &gegl_random_primes,
                                         cl_err);
  return cl_random_primes;
}

