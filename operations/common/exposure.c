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
 * Copyright 2012,2013 Felix Ulber <felix.ulber@gmx.de>
 *           2013 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_double_ui (exposure, _("Exposure"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, -10.0, 10.0, 1.0,
                   _("Relative brightness change in stops"))
gegl_chant_double_ui (offset, _("Offset"), -0.5, 0.5, 0.0, -0.5, 0.5, 1.0,
                   _("Offset value added"))
gegl_chant_double_ui (gamma, _("Gamma"), 0.01, 10, 1.0, 0.01, 3.0, 1.0,
                   _("Gamma correction"))
#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE         "exposure.c"

#include "gegl-chant.h"

#include <math.h>
#ifdef _MSC_VER
#define powf(a,b) ((gfloat)pow(a,b))
#endif

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

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

  gfloat      gain = powf(2.0, o->exposure);
  gfloat      offset = o->offset;
  gfloat      gamma = 1.0 / o->gamma;
  
  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;
  
  if (gamma == 1.0)
    for (i=0; i<n_pixels; i++)
      {
        out_pixel[0] = (in_pixel[0] * gain + offset);
        out_pixel[1] = (in_pixel[1] * gain + offset);
        out_pixel[2] = (in_pixel[2] * gain + offset);
        out_pixel[3] = in_pixel[3];
        
        out_pixel += 4;
        in_pixel  += 4;
      }
  else
    for (i=0; i<n_pixels; i++)
      {
        out_pixel[0] = powf(in_pixel[0] * gain + offset, gamma);
        out_pixel[1] = powf(in_pixel[1] * gain + offset, gamma);
        out_pixel[2] = powf(in_pixel[2] * gain + offset, gamma);
        out_pixel[3] = in_pixel[3];
        
        out_pixel += 4;
        in_pixel += 4;
      }
    
  return TRUE;
}

#include "opencl/gegl-cl.h"

static const char* kernel_source =
"__kernel void kernel_exposure(__global const float4 *in,     \n"
"                              __global       float4 *out,    \n"
"                              float                  gain,   \n"
"                              float                  offset, \n"
"                              float                  gamma)  \n"
"{                                                            \n"
"  int gid = get_global_id(0);                                \n"
"  float4 in_v  = in[gid];                                    \n"
"  float4 out_v;                                              \n"
"  out_v.xyz = pow((in_v.xyz * gain) + offset, 1.0/gamma);    \n"
"  out_v.w   =  in_v.w;                                       \n"
"  out[gid]  =  out_v;                                        \n"
"}                                                            \n";

static GeglClRunData *cl_data = NULL;

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

  gfloat      gain = powf(2.0, o->exposure);
  gfloat      offset = o->offset;
  gfloat      gamma = 1.0 / o->gamma;
  
  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_exposure", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }
  if (!cl_data) return 1;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_float), (void*)&gain);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&offset);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_float), (void*)&gamma);
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
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:exposure",
    "categories" , "color",
    "description", _("Changes Exposure and Contrast, mainly for use with high dynamic range images."),
    NULL);
}

#endif
