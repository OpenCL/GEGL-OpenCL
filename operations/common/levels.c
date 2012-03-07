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

gegl_chant_double_ui (in_low, _("Low input"), -1.0, 4.0, 0.0, 0.0, 1.0, 1.0,
                   _("Input luminance level to become lowest output"))
gegl_chant_double_ui (in_high, _("High input"), -1.0, 4.0, 1.0, 0.0, 1.0, 1.0,
                   _("Input luminance level to become white."))
gegl_chant_double_ui (out_low, _("Low output"), -1.0, 4.0, 0.0, 0.0, 1.0, 1.0,
                   _("Lowest luminance level in output"))
gegl_chant_double_ui (out_high, _("High output"), -1.0, 4.0, 1.0, 0.0, 1.0, 1.0,
                   _("Highest luminance level in output"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE         "levels.c"

#include "gegl-chant.h"

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      in_range;
  gfloat      out_range;
  gfloat      in_offset;
  gfloat      out_offset;
  gfloat      scale;
  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;

  in_offset = o->in_low * 1.0;
  out_offset = o->out_low * 1.0;
  in_range = o->in_high-o->in_low;
  out_range = o->out_high-o->out_low;

  if (in_range == 0.0)
    in_range = 0.00000001;

  scale = out_range/in_range;

  for (i=0; i<n_pixels; i++)
    {
      int c;
      for (c=0;c<3;c++)
        out_pixel[c] = (in_pixel[c]- in_offset) * scale + out_offset;
      out_pixel[3] = in_pixel[3];
      out_pixel += 4;
      in_pixel += 4;
    }
  return TRUE;
}

#include "opencl/gegl-cl.h"

static const char* kernel_source =
"__kernel void kernel_levels(__global const float4     *in,      \n"
"                            __global       float4     *out,     \n"
"                            float in_offset,                    \n"
"                            float out_offset,                   \n"
"                            float scale)                        \n"
"{                                                               \n"
"  int gid = get_global_id(0);                                   \n"
"  float4 in_v  = in[gid];                                       \n"
"  float4 out_v;                                                 \n"
"  out_v.xyz = (in_v.xyz - in_offset) * scale + out_offset;      \n"
"  out_v.w   =  in_v.w;                                          \n"
"  out[gid]  =  out_v;                                           \n"
"}                                                               \n";

static gegl_cl_run_data *cl_data = NULL;

/* OpenCL processing function */
static cl_int
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  /* Retrieve a pointer to GeglChantO structure which contains all the
   * chanted properties
   */

  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);

  gfloat      in_range;
  gfloat      out_range;
  gfloat      in_offset;
  gfloat      out_offset;
  gfloat      scale;

  cl_int cl_err = 0;

  in_offset = o->in_low * 1.0;
  out_offset = o->out_low * 1.0;
  in_range = o->in_high-o->in_low;
  out_range = o->out_high-o->out_low;

  if (in_range == 0.0)
    in_range = 0.00000001;

  scale = out_range/in_range;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_levels", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }
  if (!cl_data) return 1;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_float), (void*)&in_offset);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&out_offset);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_float), (void*)&scale);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  return cl_err;
}



static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  point_filter_class->cl_process = cl_process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:levels",
    "categories" , "color",
    "description", _("Remaps the intensity range of the image"),
    NULL);
}

#endif
