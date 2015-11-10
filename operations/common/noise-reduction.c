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
 * Ali Alsam, Hans Jakob Rivertz, Øyvind Kolås (c) 2011
 */
#include "config.h"
#include <glib/gi18n-lib.h>

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int  (iterations, _("Strength"), 4)
  description (_("Controls the number of iterations; lower values give less plastic results"))
  value_range (0, 32)
  ui_range    (0, 8)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE noise-reduction.c

#include "gegl-op.h"
#include <math.h>

/* The core noise_reduction function, which is implemented as
 * portable C - this is the function where most cpu time goes
 */
static void
noise_reduction (float *src_buf,     /* source buffer, one pixel to the left
                                        and up from the starting pixel */
                 int    src_stride,  /* stridewidth of buffer in pixels */
                 float *dst_buf,     /* destination buffer */
                 int    dst_width,   /* width to render */
                 int    dst_height,  /* height to render */
                 int    dst_stride)  /* stride of target buffer */
{
  int c;
  int x,y;
  int dst_offset;

#define NEIGHBOURS 8
#define AXES       (NEIGHBOURS/2)

#define POW2(a) ((a)*(a))
/* core code/formulas to be tweaked for the tuning the implementation */
#define GEN_METRIC(before, center, after) \
                   POW2((center) * 2 - (before) - (after))

/* Condition used to bail diffusion from a direction */
#define BAIL_CONDITION(new,original) ((new) > (original))

#define SYMMETRY(a)  (NEIGHBOURS - (a) - 1) /* point-symmetric neighbour pixel */

#define O(u,v) (((u)+((v) * src_stride)) * 4)
  int   offsets[NEIGHBOURS] = {  /* array of the relative distance i float
                                  * pointers to each of neighbours
                                  * in source buffer, allows quick referencing.
                                  */
              O( -1, -1), O(0, -1), O(1, -1),
              O( -1,  0),           O(1,  0),
              O( -1,  1), O(0, 1),  O(1,  1)};
#undef O

  dst_offset = 0;
  for (y=0; y<dst_height; y++)
    {
      float *center_pix = src_buf + ((y+1) * src_stride + 1) * 4;
      dst_offset = dst_stride * y;
      for (x=0; x<dst_width; x++)
        {
          for (c=0; c<3; c++) /* do each color component individually */
            {
              float  metric_reference[AXES];
              int    axis;
              int    direction;
              float  sum;
              int    count;

              for (axis = 0; axis < AXES; axis++)
                { /* initialize original metrics for the horizontal, vertical
                     and 2 diagonal metrics */
                  float *before_pix  = center_pix + offsets[axis];
                  float *after_pix   = center_pix + offsets[SYMMETRY(axis)];

                  metric_reference[axis] =
                    GEN_METRIC (before_pix[c], center_pix[c], after_pix[c]);
                }

              sum   = center_pix[c];
              count = 1;

              /* try smearing in data from all neighbours */
              for (direction = 0; direction < NEIGHBOURS; direction++)
                {
                  float *pix   = center_pix + offsets[direction];
                  float  value = pix[c] * 0.5 + center_pix[c] * 0.5;
                  int    axis;
                  int    valid;

                  /* check if the non-smoothing operating check is true if
                   * smearing from this direction for any of the axes */
                  valid = 1; /* assume it will be valid */
                  for (axis = 0; axis < AXES; axis++)
                    {
                      float *before_pix = center_pix + offsets[axis];
                      float *after_pix  = center_pix + offsets[SYMMETRY(axis)];
                      float  metric_new =
                             GEN_METRIC (before_pix[c], value, after_pix[c]);

                      if (BAIL_CONDITION(metric_new, metric_reference[axis]))
                        {
                          valid = 0; /* mark as not a valid smoothing, and .. */
                          break;     /* .. break out of loop */
                        }
                    }
                  if (valid) /* we were still smooth in all axes */
                    {        /* add up contribution to final result  */
                      sum += value;
                      count ++;
                    }
                }
              dst_buf[dst_offset*4+c] = sum / count;
            }
          dst_buf[dst_offset*4+3] = center_pix[3]; /* copy alpha unmodified */
          dst_offset++;
          center_pix += 4;
        }
    }
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties              *o = GEGL_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = o->iterations;
  gegl_operation_set_format (operation, "input",  babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/noise-reduction.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_noise_reduction (cl_mem                in_tex,
                    cl_mem                aux_tex,
                    cl_mem                out_tex,
                    size_t                global_worksize,
                    const GeglRectangle  *src_roi,
                    const GeglRectangle  *roi,
                    const int             iterations)
{
  int i = 0;
  size_t gbl_size_tmp[2];

  cl_int n_src_stride  = roi->width + iterations * 2;
  cl_int cl_err = 0;

  cl_mem temp_tex = NULL;
  cl_mem tmptex   = NULL;

  gint stride = 16; /*R'G'B'A float*/

  if (!cl_data)
    {
      const char *kernel_name[] ={"noise_reduction_cl","transfer", NULL};
      cl_data = gegl_cl_compile_and_build(noise_reduction_cl_source, kernel_name);
    }
  if (!cl_data)  return TRUE;

  temp_tex = gegl_clCreateBuffer (gegl_cl_get_context(),
                                  CL_MEM_READ_WRITE,
                                  src_roi->width * src_roi->height * stride,
                                  NULL, &cl_err);
  CL_CHECK;

  cl_err = gegl_clEnqueueCopyBuffer(gegl_cl_get_command_queue(),
                                    in_tex , temp_tex , 0 , 0 ,
                                    src_roi->width * src_roi->height * stride,
                                    0, NULL, NULL);
  CL_CHECK;

  tmptex = temp_tex;
  for (i = 0;i<iterations;i++)
    {
      if (i > 0)
      {
        cl_mem temp = aux_tex;
        aux_tex     = temp_tex;
        temp_tex    = temp;
      }
      gbl_size_tmp[0] = roi->width  + 2 * (iterations - 1 -i);
      gbl_size_tmp[1] = roi->height + 2 * (iterations - 1 -i);

      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&temp_tex);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_int), (void*)&n_src_stride);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem), (void*)&aux_tex);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int), (void*)&n_src_stride);
      CL_CHECK;

      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(), cl_data->kernel[0],
                                           2, NULL, gbl_size_tmp, NULL,
                                           0, NULL, NULL);
      CL_CHECK;
    }

  gbl_size_tmp[0] = roi->width ;
  gbl_size_tmp[1] = roi->height;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem), (void*)&aux_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_int), (void*)&n_src_stride);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_mem), (void*)&out_tex);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(), cl_data->kernel[1],
                                       2, NULL, gbl_size_tmp, NULL,
                                       0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish(gegl_cl_get_command_queue());
  CL_CHECK;

  if (tmptex)
    {
      cl_err = gegl_clReleaseMemObject (tmptex);
      CL_CHECK_ONLY (cl_err);
    }

  return FALSE;

error:
  if (tmptex)
    gegl_clReleaseMemObject (tmptex);

  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
      GeglBuffer                *input,
      GeglBuffer                *output,
      const GeglRectangle       *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err = 0;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  gint aux = gegl_buffer_cl_iterator_add_aux (i,
                                              result,
                                              in_format,
                                              op_area->left,
                                              op_area->right,
                                              op_area->top,
                                              op_area->bottom);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_noise_reduction (i->tex[read],
                                i->tex[aux],
                                i->tex[0],
                                i->size[0],
                                &i->roi[read],
                                &i->roi[0],
                                o->iterations);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  return !err;
}

#define INPLACE 1

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);

  int iteration;
  int stride;
  float *src_buf;
#ifndef INPLACE
  float *dst_buf;
#endif
  GeglRectangle rect;

  if (gegl_operation_use_opencl (operation))
    if(cl_process(operation, input, output, result))
      return TRUE;

  rect = *result;

  stride = result->width + o->iterations * 2;

  src_buf = g_new0 (float,
         (stride) * (result->height + o->iterations * 2) * 4);
#ifndef INPLACE
  dst_buf = g_new0 (float,
         (stride) * (result->height + o->iterations * 2) * 4);
#endif

  {
    rect.x      -= o->iterations;
    rect.y      -= o->iterations;
    rect.width  += o->iterations*2;
    rect.height += o->iterations*2;
    gegl_buffer_get (input, &rect, 1.0, babl_format ("R'G'B'A float"),
                     src_buf, stride * 4 * 4, GEGL_ABYSS_NONE);
  }

  for (iteration = 0; iteration < o->iterations; iteration++)
    {
      noise_reduction (src_buf, stride,
#ifdef INPLACE
                       src_buf + (stride + 1) * 4,
#else
                       dst_buf,
#endif
                       result->width  + (o->iterations - 1 - iteration) * 2,
                       result->height + (o->iterations - 1 - iteration) * 2,
                       stride);
#ifndef INPLACE
      { /* swap buffers */
        float *tmp = src_buf;
        src_buf = dst_buf;
        dst_buf = tmp;
      }
#endif
    }

  gegl_buffer_set (output, result, 0, babl_format ("R'G'B'A float"),
#ifndef INPLACE
                   src_buf,
#else
                   src_buf + ((stride +1) * 4) * o->iterations,
#endif
                   stride * 4 * 4);

  g_free (src_buf);
#ifndef INPLACE
  g_free (dst_buf);
#endif

  return  TRUE;
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");
  if (!in_rect)
    return result;
  return *in_rect;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "title",       _("Noise Reduction"),
    "name"       , "gegl:noise-reduction",
    "categories" , "enhance:noise-reduction",
    "description", _("Anisotropic smoothing operation"),
    NULL);
}

#endif
