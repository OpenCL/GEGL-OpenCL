/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

   /* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "grey.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("YA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/* XXX: could be sped up by special casing op-filter behavior */

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  float *in = in_buf;
  float *out = out_buf;
  while (samples--)
    {
      *out++ = *in++;
      *out++ = *in++;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"

static cl_int
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  cl_int cl_err = 0;

  cl_err = gegl_clEnqueueCopyBuffer(gegl_cl_get_command_queue(),
                                    in_tex , out_tex , 0 , 0 ,
                                    global_worksize * sizeof (cl_float2),
                                    0, NULL, NULL);

  if (CL_SUCCESS != cl_err) return cl_err;

  return cl_err;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  point_filter_class->cl_process = cl_process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name"       , "gegl:grey",
      "categories" , "color",
      "description", _("Turns the image greyscale"),
      NULL);
}

#endif
