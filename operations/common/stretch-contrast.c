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

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "stretch-contrast.c"

#include "gegl-chant.h"

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gdouble    *min,
                    gdouble    *max)
{
  gfloat tmin =  G_MAXFLOAT;
  gfloat tmax = -G_MAXFLOAT;

  GeglBufferIterator *gi;
  gi = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("R'G'B' float"),
                                 GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];

      gint i;
      for (i = 0; i < gi->length * 3; i++)
        {
          gfloat val = buf [i];

          if (val < tmin)
            tmin = val;
          if (val > tmax)
            tmax = val;
        }
    }

  if (min)
    *min = tmin;
  if (max)
    *max = tmax;
}

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"
#include "opencl/stretch-contrast.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_build_kernels (void)
{
  if (!cl_data)
    {
      const char *kernel_name[] = {"two_stages_local_min_max_reduce",
                                   "global_min_max_reduce",
                                   "cl_stretch_contrast",
                                   "init_stretch",
                                   NULL};
      cl_data = gegl_cl_compile_and_build (stretch_contrast_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  return FALSE;
}

static gboolean
cl_buffer_get_min_max (cl_mem               in_tex,
                       size_t               global_worksize,
                       const GeglRectangle *roi,
                       gfloat              *min,
                       gfloat              *max)
{
  cl_int   cl_err      = 0;
  size_t   local_ws, max_local_ws;
  size_t   work_groups;
  size_t   global_ws;
  cl_mem   cl_aux_min  = NULL;
  cl_mem   cl_aux_max  = NULL;
  cl_mem   cl_min_max  = NULL;
  cl_int   n_pixels    = (cl_int)global_worksize;
  cl_float min_max_buf[2];

  if (global_worksize < 1)
    {
      *min = G_MAXFLOAT;
      *max = G_MINFLOAT;
      return FALSE;
    }

  cl_err = gegl_clGetDeviceInfo (gegl_cl_get_device (),
                                 CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof (size_t), &max_local_ws, NULL);
  CL_CHECK;

  /* Needs to be a power of two */
  local_ws = 256;
  while (local_ws > max_local_ws)
    local_ws /= 2;

  work_groups = MIN ((global_worksize + local_ws - 1) / local_ws, local_ws);
  global_ws = work_groups * local_ws;


  cl_aux_min = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_READ_WRITE,
                                    local_ws * sizeof(cl_float),
                                    NULL, &cl_err);
  CL_CHECK;
  cl_aux_max = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_READ_WRITE,
                                    local_ws * sizeof(cl_float),
                                    NULL, &cl_err);
  CL_CHECK;
  cl_min_max = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_WRITE_ONLY,
                                    2 * sizeof(cl_float),
                                    NULL, &cl_err);
  CL_CHECK;

  /* The full initialization is done in the two_stages_local_min_max_reduce
     kernel */
#if 0
  cl_err = gegl_clSetKernelArg(cl_data->kernel[3], 0, sizeof(cl_mem),
                               (void*)&cl_aux_min);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[3], 1, sizeof(cl_mem),
                               (void*)&cl_aux_max);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[3], 1,
                                       NULL, &local_ws, &local_ws,
                                       0, NULL, NULL);
  CL_CHECK;
#endif

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),
                               (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),
                               (void*)&cl_aux_min);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem),
                               (void*)&cl_aux_max);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3,
                               sizeof(cl_float) * local_ws, NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4,
                               sizeof(cl_float) * local_ws, NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_int),
                               (void*)&n_pixels);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 1,
                                       NULL, &global_ws, &local_ws,
                                       0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),
                               (void*)&cl_aux_min);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),
                               (void*)&cl_aux_max);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_mem),
                               (void*)&cl_min_max);
  CL_CHECK;

  /* Only one work group */
  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 1,
                                        NULL, &local_ws, &local_ws,
                                        0, NULL, NULL);
  CL_CHECK;

  /* Read the memory buffer, probably better to keep it in GPU memory */
  cl_err = gegl_clEnqueueReadBuffer (gegl_cl_get_command_queue (),
                                     cl_min_max, CL_TRUE, 0,
                                     2 * sizeof (cl_float), &min_max_buf, 0,
                                     NULL, NULL);
  CL_CHECK;

  *min = min_max_buf[0];
  *max = min_max_buf[1];


  cl_err = gegl_clReleaseMemObject (cl_aux_min);
  CL_CHECK;

  cl_err = gegl_clReleaseMemObject (cl_aux_max);
  CL_CHECK;

  cl_err = gegl_clReleaseMemObject (cl_min_max);
  CL_CHECK;

  return FALSE;

error:
  if (cl_aux_min)
    gegl_clReleaseMemObject (cl_aux_min);
  if (cl_aux_max)
    gegl_clReleaseMemObject (cl_aux_max);
  if (cl_min_max)
    gegl_clReleaseMemObject (cl_min_max);
  return TRUE;
}

static gboolean
cl_stretch_contrast (cl_mem               in_tex,
                     cl_mem               out_tex,
                     size_t               global_worksize,
                     const GeglRectangle *roi,
                     gfloat               min,
                     gfloat               diff)
{
  cl_int   cl_err  = 0;
  cl_float cl_min  = min;
  cl_float cl_diff = diff;

  {
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 0, sizeof(cl_mem),
                               (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 1, sizeof(cl_mem),
                               (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 2, sizeof(cl_float),
                               (void*)&cl_min);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 3, sizeof(cl_float),
                               (void*)&cl_diff);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                         cl_data->kernel[2], 1,
                                         NULL, &global_worksize, NULL,
                                         0, NULL, NULL);
  CL_CHECK;

  }
  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  gfloat min = 1.0f;
  gfloat max = 0.0f;
  gfloat i_min, i_max, diff;
  cl_int err = 0;
  gint read;
  GeglBufferClIterator *i;

  if (cl_build_kernels ())
    return FALSE;

  i = gegl_buffer_cl_iterator_new (input,
                                   result,
                                   in_format,
                                   GEGL_CL_BUFFER_READ);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_buffer_get_min_max (i->tex[0],
                                   i->size[0],
                                   &i->roi[0],
                                   &i_min,
                                   &i_max);
      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }

      if (i_min < min)
        min = i_min;
      if (i_max > max)
        max = i_max;
    }

  if (err)
    return FALSE;

  diff = max-min;

  i = gegl_buffer_cl_iterator_new (output,
                                   result,
                                   out_format,
                                   GEGL_CL_BUFFER_WRITE);

  read = gegl_buffer_cl_iterator_add_2 (i,
                                        input,
                                        result,
                                        in_format,
                                        GEGL_CL_BUFFER_READ,
                                        0,
                                        0,
                                        0,
                                        0,
                                        GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_stretch_contrast (i->tex[read],
                                 i->tex[0],
                                 i->size[0],
                                 &i->roi[0],
                                 min,
                                 diff);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  return !err;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  gdouble  min, max, diff;
  GeglBufferIterator *gi;

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

  buffer_get_min_max (input, &min, &max);
  diff = max - min;

  /* Avoid a divide by zero error if the image is a solid color */
  if (diff == 0.0){
    gegl_buffer_copy (input, NULL, output, NULL);
    return TRUE;
  }

  gi = gegl_buffer_iterator_new (input, result, 0, babl_format ("R'G'B'A float"),
                                 GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, babl_format ("R'G'B'A float"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *in  = gi->data[0];
      gfloat *out = gi->data[1];

      gint o;
      for (o = 0; o < gi->length; o++)
        {
          out[0] = (in[0] - min) / diff;
          out[1] = (in[1] - min) / diff;
          out[2] = (in[2] - min) / diff;
          out[3] = in[3];

          in  += 4;
          out += 4;
        }
    }

  return TRUE;
}

/* This is called at the end of the gobject class_init function.
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:stretch-contrast",
    "categories" , "color:enhance",
    "description",
        _("Scales the components of the buffer to be in the 0.0-1.0 range. "
          "This improves images that make poor use of the available contrast "
          "(little contrast, very dark, or very bright images)."),
        NULL);
}

#endif
