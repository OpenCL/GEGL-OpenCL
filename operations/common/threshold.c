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

gegl_chant_double_ui (value, _("Threshold"), -10.0, 10.0, 0.5, 0.0, 1.0, 1.0, 
   _("Global threshold level (used when there is no auxiliary input buffer)."))

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE       "threshold.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("YA float"));
  gegl_operation_set_format (operation, "aux", babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  glong   i;

  if (aux == NULL)
    {
      gfloat value = GEGL_CHANT_PROPERTIES (op)->value;
      for (i=0; i<n_pixels; i++)
        {
          gfloat c;

          c = in[0];
          c = c>=value?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gfloat value = *aux;
          gfloat c;

          c = in[0];
          c = c>=value?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
          aux += 1;
        }
    }
  return TRUE;
}

#include "opencl/gegl-cl.h"

static const char* kernel_source =
"__kernel void kernel_thr_3 (__global const float2     *in,     \n"
"                            __global const float      *aux,    \n"
"                            __global       float2     *out)    \n"
"{                                                              \n"
"  int gid = get_global_id(0);                                  \n"
"  float2 in_v  = in [gid];                                     \n"
"  float  aux_v = aux[gid];                                     \n"
"  float2 out_v;                                                \n"
"  out_v.x = (in_v.x > aux_v)? 1.0f : 0.0f;                     \n"
"  out_v.y = in_v.y;                                            \n"
"  out[gid]  =  out_v;                                          \n"
"}                                                              \n"

"__kernel void kernel_thr_2 (__global const float2     *in,     \n"
"                            __global       float2     *out,    \n"
"                            float value)                       \n"
"{                                                              \n"
"  int gid = get_global_id(0);                                  \n"
"  float2 in_v  = in [gid];                                     \n"
"  float2 out_v;                                                \n"
"  out_v.x = (in_v.x > value)? 1.0f : 0.0f;                     \n"
"  out_v.y = in_v.y;                                            \n"
"  out[gid]  =  out_v;                                          \n"
"}                                                              \n";

static gegl_cl_run_data *cl_data = NULL;

/* OpenCL processing function */
static cl_int
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               aux_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  gfloat value = GEGL_CHANT_PROPERTIES (op)->value;

  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_thr_3", "kernel_thr_2", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }

  if (!cl_data) return 1;

  if (aux_tex)
    {
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem),   (void*)&out_tex);
      if (cl_err != CL_SUCCESS) return cl_err;

      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                            cl_data->kernel[0], 1,
                                            NULL, &global_worksize, NULL,
                                            0, NULL, NULL);
      if (cl_err != CL_SUCCESS) return cl_err;
    }
  else
    {
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),   (void*)&in_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),   (void*)&out_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_float), (void*)&value);
      if (cl_err != CL_SUCCESS) return cl_err;

      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                            cl_data->kernel[1], 1,
                                            NULL, &global_worksize, NULL,
                                            0, NULL, NULL);
      if (cl_err != CL_SUCCESS) return cl_err;
    }

  return cl_err;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  point_composer_class->cl_process = cl_process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:threshold",
    "categories" , "color",
    "description",
          _("Thresholds the image to white/black based on either the global value "
            "set in the value property, or per pixel from the aux input."),
          NULL);
}

#endif
