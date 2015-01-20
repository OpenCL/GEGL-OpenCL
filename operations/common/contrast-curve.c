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
 * Copyright 2007 Mark Probst <mark.probst@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_int (sampling_points, _("Sample points"), 0)
  description (_("Number of curve sampling points.  0 for exact calculation."))
  value_range (0, 65536)

property_curve (curve, _("Curve"), NULL)
  description (_("The contrast curve."))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE contrast-curve.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("YA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

#include "opencl/gegl-cl.h"
#include "gegl/gegl-debug.h"

#include "opencl/contrast-curve.cl.h"

/* TODO : Replace gegl_curve_calc_values and gegl_curve_calc_value in cl_process
          with something more suitable for the cl version*/

static void
copy_double_array_to_float_array (gdouble *in,
                                  gfloat  *out,
                                  gint     size)
{
  gint i;
  for (i = 0; i < size; ++i)
    out[i] = (gfloat) in[i];
}

static GeglClRunData * cl_data = NULL;

static gboolean
cl_process (GeglOperation       *self,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (self);
  gint       num_sampling_points;
  gdouble    *xs, *ys;
  gfloat     *ysf = NULL;
  cl_mem     cl_curve = NULL;
  cl_ulong   cl_max_constant_size;
  cl_int     cl_err = 0;

  num_sampling_points = o->sampling_points;

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_contrast_curve",NULL};
      cl_data = gegl_cl_compile_and_build (contrast_curve_cl_source,
                                           kernel_name);
    }
  if (!cl_data)
    return TRUE;

  if (num_sampling_points > 0)
    {
      xs = g_new (gdouble, num_sampling_points);
      ys = g_new (gdouble, num_sampling_points);

      gegl_curve_calc_values (o->curve, 0.0, 1.0, num_sampling_points, xs, ys);
      g_free (xs);

      /*We need to downscale the array to pass it to the GPU*/
      ysf = g_new (gfloat, num_sampling_points);
      copy_double_array_to_float_array (ys, ysf, num_sampling_points);
      g_free (ys);

      cl_err = gegl_clGetDeviceInfo (gegl_cl_get_device (),
                                     CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
                                     sizeof (cl_ulong),
                                     &cl_max_constant_size,
                                     NULL);
      CL_CHECK;

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Max Constant Mem Size: %lu bytes",
                 (unsigned long) cl_max_constant_size);

      if (sizeof (cl_float) * num_sampling_points < cl_max_constant_size)
        {
          cl_curve = gegl_clCreateBuffer (gegl_cl_get_context (),
                                          CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          num_sampling_points * sizeof (cl_float),
                                          ysf, &cl_err);
          CL_CHECK;
          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 0, sizeof (cl_mem),
                                        (void*) &in_tex);
          CL_CHECK;
          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 1, sizeof (cl_mem),
                                        (void*) &out_tex);
          CL_CHECK;
          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 2, sizeof (cl_mem),
                                        (void*) &cl_curve);
          CL_CHECK;
          cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 3, sizeof (gint),
                                        (void*) &num_sampling_points);
          CL_CHECK;
          cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                                cl_data->kernel[0], 1,
                                                NULL, &global_worksize, NULL,
                                                0, NULL, NULL);
          CL_CHECK;

          cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
          CL_CHECK;

          cl_err = gegl_clReleaseMemObject (cl_curve);
          CL_CHECK_ONLY (cl_err);
        }
      else
        {
          /*If the curve size doesn't fit constant memory is better to use CPU*/
          GEGL_NOTE (GEGL_DEBUG_OPENCL,
                     "Not enough constant memory for the curve");
          g_free (ysf);
          return TRUE;
        }

      g_free (ysf);
      return FALSE;
error:
      if (ysf)
        g_free (ysf);
      if (cl_curve)
        gegl_clReleaseMemObject (cl_curve);

      return TRUE;
    }
  else  /*If the curve doesn't have a lookup table is better to use CPU*/
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL,
                 "Curve not suitable to be computed in the GPU");
      return TRUE;
    }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gint        num_sampling_points;
  GeglCurve  *curve;
  gint i;
  gfloat  *in  = in_buf;
  gfloat  *out = out_buf;
  gdouble *xs, *ys;

  num_sampling_points = o->sampling_points;
  curve = o->curve;

  if (num_sampling_points > 0)
  {
    xs = g_new(gdouble, num_sampling_points);
    ys = g_new(gdouble, num_sampling_points);

    gegl_curve_calc_values(o->curve, 0.0, 1.0, num_sampling_points, xs, ys);

    g_free(xs);

    for (i=0; i<samples; i++)
    {
      gint x = in[0] * num_sampling_points;
      gfloat y;

      if (x < 0)
       y = ys[0];
      else if (x >= num_sampling_points)
       y = ys[num_sampling_points - 1];
      else
       y = ys[x];

      out[0] = y;
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

    g_free(ys);
  }
  else
    for (i=0; i<samples; i++)
    {
      gfloat u = in[0];

      out[0] = gegl_curve_calc_value(curve, u);
      out[1]=in[1];

      in += 2;
      out+= 2;
    }

  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  point_filter_class->cl_process = cl_process;
  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:contrast-curve",
    "title",       _("Contrast Curve"),
    "categories" , "color",
    "description",
        _("Adjusts the contrast of a grayscale image with a curve specifying contrast for intensity."),
        NULL);
}

#endif
