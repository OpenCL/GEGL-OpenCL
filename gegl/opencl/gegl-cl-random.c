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
#include "gegl-random-private.h"

static cl_mem cl_random_data = NULL;

cl_mem
gegl_cl_load_random_data (gint *cl_err)
{
  if (cl_random_data == NULL)
    {
      guint32 *random_data;

      random_data = gegl_random_get_data ();
      cl_random_data = gegl_clCreateBuffer (gegl_cl_get_context (),
                                            CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                            RANDOM_DATA_SIZE * sizeof (guint32),
                                            (void*) random_data,
                                            cl_err);
    }
  else
    {
      *cl_err = CL_SUCCESS;
    }
  return cl_random_data;
}

cl_int
gegl_cl_random_cleanup (void)
{
  cl_int cl_err = CL_SUCCESS;
  if (cl_random_data != NULL)
    {
      cl_err = gegl_clReleaseMemObject (cl_random_data);
      cl_random_data = NULL;
    }
  return cl_err;
}

void
gegl_cl_random_get_ushort4 (const GeglRandom *in_rand,
                                  cl_ushort4 *out_rand)
{
  out_rand->x = in_rand->prime0;
  out_rand->y = in_rand->prime1;
  out_rand->z = in_rand->prime2;
  out_rand->w = 0;
}
