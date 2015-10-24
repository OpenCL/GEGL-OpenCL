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
 * Copyright 1996 Federico Mena Quintero <federico@nuclecu.unam.mx>
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (keep_colors, _("Keep colors"), TRUE)
    description(_("Impact each channel with the same amount"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE stretch-contrast.c

#include "gegl-op.h"
#include <math.h>

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gfloat     *min,
                    gfloat     *max)
{
  GeglBufferIterator *gi;
  gint c;
  gi = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("RGB float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  for (c = 0; c < 3; c++)
    {
      min[c] =  G_MAXFLOAT;
      max[c] = -G_MAXFLOAT;
    }

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];

      gint i;
      for (i = 0; i < gi->length; i++)
        {
          for (c = 0; c < 3; c++)
            {
              min[c] = MIN (buf [i * 3 + c], min[c]);
              max[c] = MAX (buf [i * 3 + c], max[c]);
            }
        }
    }
}

static void
reduce_min_max_global (gfloat *min,
                       gfloat *max)
{
  gfloat vmin, vmax;
  gint   c;

  vmin= min[0];
  vmax= max[0];

  for (c = 1; c < 3; c++)
    {
      vmin = MIN (min[c], vmin);
      vmax = MAX (max[c], vmax);
    }

  for (c = 0; c < 3; c++)
    {
      min[c] = vmin;
      max[c] = vmax;
    }
}

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

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
                       gfloat              min[4],
                       gfloat              max[4])
{
  cl_int   cl_err      = 0;
  size_t   local_ws, max_local_ws;
  size_t   work_groups;
  size_t   global_ws;
  cl_mem   cl_aux_min  = NULL;
  cl_mem   cl_aux_max  = NULL;
  cl_mem   cl_min_max  = NULL;
  cl_int   n_pixels    = (cl_int)global_worksize;
  cl_float4 min_max_buf[2];

  if (global_worksize < 1)
    {
      min[0] = min[1] = min[2] = min[3] =  G_MAXFLOAT;
      max[0] = max[1] = max[2] = max[3] = -G_MAXFLOAT;
      return FALSE;
    }

  cl_err = gegl_clGetDeviceInfo (gegl_cl_get_device (),
                                 CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof (size_t), &max_local_ws, NULL);
  CL_CHECK;

  max_local_ws = MIN (max_local_ws,
                      MIN (cl_data->work_group_size[0],
                           cl_data->work_group_size[1]));

  /* Needs to be a power of two */
  local_ws = 256;
  while (local_ws > max_local_ws)
    local_ws /= 2;

  work_groups = MIN ((global_worksize + local_ws - 1) / local_ws, local_ws);
  global_ws = work_groups * local_ws;


  cl_aux_min = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_READ_WRITE,
                                    local_ws * sizeof(cl_float4),
                                    NULL, &cl_err);
  CL_CHECK;
  cl_aux_max = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_READ_WRITE,
                                    local_ws * sizeof(cl_float4),
                                    NULL, &cl_err);
  CL_CHECK;
  cl_min_max = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_WRITE_ONLY,
                                    2 * sizeof(cl_float4),
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
                               sizeof(cl_float4) * local_ws, NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4,
                               sizeof(cl_float4) * local_ws, NULL);
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
                                     2 * sizeof (cl_float4), &min_max_buf, 0,
                                     NULL, NULL);
  CL_CHECK;

  min[0] = min_max_buf[0].x;
  min[1] = min_max_buf[0].y;
  min[2] = min_max_buf[0].z;
  min[3] = min_max_buf[0].w;

  max[0] = min_max_buf[1].x;
  max[1] = min_max_buf[1].y;
  max[2] = min_max_buf[1].z;
  max[3] = min_max_buf[1].w;

  cl_err = gegl_clReleaseMemObject (cl_aux_min);
  CL_CHECK_ONLY (cl_err);

  cl_err = gegl_clReleaseMemObject (cl_aux_max);
  CL_CHECK_ONLY (cl_err);

  cl_err = gegl_clReleaseMemObject (cl_min_max);
  CL_CHECK_ONLY (cl_err);

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
                     cl_float4            min,
                     cl_float4            diff)
{
  cl_int cl_err  = 0;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 0, sizeof(cl_mem),
                               (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 1, sizeof(cl_mem),
                               (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 2, sizeof(cl_float4),
                               (void*)&min);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[2], 3, sizeof(cl_float4),
                               (void*)&diff);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[2], 1,
                                       NULL, &global_worksize, NULL,
                                       0, NULL, NULL);
  CL_CHECK;

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

  gfloat    min[] = {1.0f, 1.0f, 1.0f, 1.0f};
  gfloat    max[] = {0.0f, 0.0f, 0.0f, 0.0f};
  gfloat    i_min[4], i_max[4], diff[4];
  cl_int    err = 0;
  gint      read, c;
  GeglBufferClIterator *i;
  GeglProperties           *o;
  cl_float4 cl_min, cl_diff;

  o = GEGL_PROPERTIES (operation);

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
                                   i_min,
                                   i_max);
      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }

      for (c = 0; c < 3; c++)
        {
          if (i_min[c] < min[c])
            min[c] = i_min[c];
          if (i_max[c] > max[c])
            max[c] = i_max[c];
        }
    }

  if (err)
    return FALSE;

  if (o->keep_colors)
    reduce_min_max_global (min, max);

  for (c = 0; c < 3; c ++)
    {
      diff[c] = max[c] - min[c];

      /* Avoid a divide by zero error if the image is a solid color */
      if (diff[c] < 1e-3)
        {
          min[c]  = 0.0;
          diff[c] = 1.0;
        }
    }

  cl_diff.x = diff[0];
  cl_diff.y = diff[1];
  cl_diff.z = diff[2];
  cl_diff.w = 1.0;

  cl_min.x = min[0];
  cl_min.y = min[1];
  cl_min.z = min[2];
  cl_min.w = 0.0;

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
                                 cl_min,
                                 cl_diff);

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
  gfloat  min[3], max[3], diff[3];
  GeglBufferIterator *gi;
  GeglProperties         *o;
  gint                c;

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

  o = GEGL_PROPERTIES (operation);

  buffer_get_min_max (input, min, max);

  if (o->keep_colors)
    reduce_min_max_global (min, max);

  for (c = 0; c < 3; c++)
    {
      diff[c] = max[c] - min[c];

      /* Avoid a divide by zero error if the image is a solid color */
      if (diff[c] < 1e-3)
        {
          min[c]  = 0.0;
          diff[c] = 1.0;
        }
    }

  gi = gegl_buffer_iterator_new (input, result, 0, babl_format ("RGBA float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, babl_format ("RGBA float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *in  = gi->data[0];
      gfloat *out = gi->data[1];

      gint o;
      for (o = 0; o < gi->length; o++)
        {
          for (c = 0; c < 3; c++)
            out[c] = (in[c] - min[c]) / diff[c];

          out[3] = in[3];

          in  += 4;
          out += 4;
        }
    }

  return TRUE;
}

/* Pass-through when trying to perform a reduction on an infinite plane
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
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

/* This is called at the end of the gobject class_init function.
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:stretch-contrast",
    "title",       _("Stretch Contrast"),
    "categories" , "color:enhance",
    "description",
        _("Scales the components of the buffer to be in the 0.0-1.0 range. "
          "This improves images that make poor use of the available contrast "
          "(little contrast, very dark, or very bright images)."),
        NULL);
}

#endif
