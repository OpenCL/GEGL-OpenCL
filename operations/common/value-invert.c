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
 * Copyright 2007 Mukund Sivaraman <muks@mukund.org>
 */

/* XXX
 * This plug-in isn't really useful apart for compatibility with GIMP
 * scripts, operating with HSV as a color model for filters isn't really
 * useful.
 */

/*
 * The plug-in only does v = 1.0 - v; for each pixel in the image, or
 * each entry in the colormap depending upon the type of image, where 'v'
 * is the value in HSV color model.
 *
 * The plug-in code is optimized towards this, in that it is not a full
 * RGB->HSV->RGB transform, but shortcuts many of the calculations to
 * effectively only do v = 1.0 - v. In fact, hue is never calculated. The
 * shortcuts can be derived from running a set of r, g, b values through the
 * RGB->HSV transform and then from HSV->RGB and solving out the redundant
 * portions.
 *
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

   /* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "value-invert.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  glong   j;
  gfloat *src  = in_buf;
  gfloat *dest = out_buf;
  gfloat  r, g, b;
  gfloat  value, min;
  gfloat  delta;

  for (j = 0; j < samples; j++)
    {
      r = *src++;
      g = *src++;
      b = *src++;

      if (r > g)
        {
          value = MAX (r, b);
          min = MIN (g, b);
        }
      else
        {
          value = MAX (g, b);
          min = MIN (r, b);
        }

      delta = value - min;
      if ((value == 0) || (delta == 0))
        {
          r = 1.0 - value;
          g = 1.0 - value;
          b = 1.0 - value;
        }
      else
        {
          if (r == value)
            {
              r = 1.0 - r;
              b = r * b / value;
              g = r * g / value;
            }
          else if (g == value)
            {
              g = 1.0 - g;
              r = g * r / value;
              b = g * b / value;
            }
          else
            {
              b = 1.0 - b;
              g = b * g / value;
              r = b * r / value;
            }
        }

      *dest++ = r;
      *dest++ = g;
      *dest++ = b;

      *dest++ = *src++;
    }
  return TRUE;
}

#include "opencl/gegl-cl.h"

static const char* kernel_source =
"__kernel void kernel_vinv(__global const float4     *in,           \n"
"                          __global       float4     *out)          \n"
"{                                                                  \n"
"  int gid = get_global_id(0);                                      \n"
"  float4 in_v  = in[gid];                                          \n"
"  float4 out_v;                                                    \n"
"                                                                   \n"
"  float value = fmax (in_v.x, fmax (in_v.y, in_v.z));              \n"
"  float minv  = fmin (in_v.x, fmin (in_v.y, in_v.z));              \n"
"  float delta = value - minv;                                      \n"
"                                                                   \n"
"  if (value == 0.0f || delta == 0.0f)                              \n"
"    {                                                              \n"
"      out_v.xyz = (float3) (1.0f - value);                         \n"
"    }                                                              \n"
"  else                                                             \n"
"    {                                                              \n"
"      if (in_v.x == value)                                         \n"
"        {                                                          \n"
"          out_v.xzy = (float3) ((1.0f - in_v.x),                   \n"
"                                (1.0f - in_v.x) * in_v.z / value,  \n"
"                                (1.0f - in_v.x) * in_v.y / value); \n"
"        }                                                          \n"
"      else if (in_v.y == value)                                    \n"
"        {                                                          \n"
"          out_v.yxz = (float3) ((1.0f - in_v.y),                   \n"
"                                (1.0f - in_v.y) * in_v.x / value,  \n"
"                                (1.0f - in_v.y) * in_v.z / value); \n"
"        }                                                          \n"
"      else                                                         \n"
"        {                                                          \n"
"          out_v.zyx = (float3) ((1.0f - in_v.z),                   \n"
"                                (1.0f - in_v.z) * in_v.y / value,  \n"
"                                (1.0f - in_v.z) * in_v.x / value); \n"
"        }                                                          \n"
"    }                                                              \n"
"                                                                   \n"
"  out_v.w   =  in_v.w;                                             \n"
"  out[gid]  =  out_v;                                              \n"
"}                                                                  \n";

static gegl_cl_run_data *cl_data = NULL;

/* OpenCL processing function */
static cl_int
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            int                  level)
{

  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_vinv", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }

  if (!cl_data) return 1;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
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
  operation_class->prepare = prepare;
  point_filter_class->cl_process = cl_process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:value-invert",
    "categories" , "color",
    "description",
        _("Inverts just the value component, the result is the corresponding "
          "`inverted' image."),
        NULL);
}

#endif

