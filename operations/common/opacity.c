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

gegl_chant_double (value, _("Opacity"), -10.0, 10.0, 1.0,
         _("Global opacity value that is always used on top of the optional auxiliary input buffer."))

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE       "opacity.c"

#include "gegl-chant.h"


static void prepare (GeglOperation *self)
{
  gegl_operation_set_format (self, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (self, "output", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (self, "aux", babl_format ("Y float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gfloat value = GEGL_CHANT_PROPERTIES (op)->value;

  if (aux == NULL)
    {
      g_assert (value != 1.0); /* buffer should have been passed through */
      while (samples--)
        {
          gint j;
          for (j=0; j<4; j++)
            out[j] = in[j] * value;
          in  += 4;
          out += 4;
        }
    }
  else if (value == 1.0)
    while (samples--)
      {
        gint j;
        for (j=0; j<4; j++)
          out[j] = in[j] * (*aux);
        in  += 4;
        out += 4;
        aux += 1;
      }
  else
    while (samples--)
      {
        gfloat v = (*aux) * value;
        gint j;
        for (j=0; j<4; j++)
          out[j] = in[j] * v;
        in  += 4;
        out += 4;
        aux += 1;
      }
  return TRUE;
}

#include "opencl/gegl-cl.h"

static const char* kernel_source =
"__kernel void kernel_op_3 (__global const float4     *in,      \n"
"                           __global const float      *aux,     \n"
"                           __global       float4     *out,     \n"
"                           float value)                        \n"
"{                                                              \n"
"  int gid = get_global_id(0);                                  \n"
"  float4 in_v  = in [gid];                                     \n"
"  float  aux_v = aux[gid];                                     \n"
"  float4 out_v;                                                \n"
"  out_v = in_v * aux_v * value;                                \n"
"  out[gid]  =  out_v;                                          \n"
"}                                                              \n"

"__kernel void kernel_op_2 (__global const float4     *in,      \n"
"                           __global       float4     *out,     \n"
"                           float value)                        \n"
"{                                                              \n"
"  int gid = get_global_id(0);                                  \n"
"  float4 in_v  = in [gid];                                     \n"
"  float4 out_v;                                                \n"
"  out_v = in_v * value;                                        \n"
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
      const char *kernel_name[] = {"kernel_op_3", "kernel_op_2", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }

  if (!cl_data) return 1;

  if (aux_tex)
    {
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem),   (void*)&out_tex);
      cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&value);
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

/* Fast path when opacity is a no-op
 */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GeglOperationClass  *operation_class;
  gpointer in, aux;
  operation_class = GEGL_OPERATION_CLASS (gegl_chant_parent_class);

  /* get the raw values this does not increase the reference count */
  in = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  if (in && !aux && GEGL_CHANT_PROPERTIES (operation)->value == 1.0)
    {
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result, 
                                  gegl_operation_context_get_level (context));
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  point_composer_class->process = process;
  point_composer_class->cl_process = cl_process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:opacity",
    "categories" , "transparency",
    "description",
          _("Weights the opacity of the input both the value of the aux"
            " input and the global value property."),
    NULL);
}

#endif
