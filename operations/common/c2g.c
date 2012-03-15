/* STRESS, Spatio Temporal Retinex Envelope with Stochastic Sampling
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
 * Copyright 2007,2009 Øyvind Kolås     <pippin@gimp.org>
 *                     Ivar Farup       <ivarf@hig.no>
 *                     Allesandro Rizzi <rizzi@dti.unimi.it>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int_ui (radius, _("Radius"), 2, 3000, 300, 2, 3000, 1.6,
                _("Neighborhood taken into account, this is the radius in pixels taken into account when deciding which colors map to which gray values"))
gegl_chant_int_ui (samples, _("Samples"), 1, 1000, 4, 1, 20, 1.0,
                _("Number of samples to do per iteration looking for the range of colors"))
gegl_chant_int_ui (iterations, _("Iterations"), 1, 1000, 10, 1, 20, 1.0,
                _("Number of iterations, a higher number of iterations provides less noisy results at a computational cost"))

/*
gegl_chant_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                _("Gamma applied to radial distribution"))
*/
#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "c2g.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>
#include "envelopes.h"

#define RGAMMA 2.0

static void c2g (GeglBuffer          *src,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *dst,
                 const GeglRectangle *dst_rect,
                 gint                 radius,
                 gint                 samples,
                 gint                 iterations,
                 gdouble              rgamma)
{
  gint x,y;
  gint    dst_offset=0;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint    inw = src_rect->width;
  gint    outw = dst_rect->width;
  gint    inh = src_rect->height;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 2);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  for (y=radius; y<dst_rect->height+radius; y++)
    {
      gint src_offset = (inw*y+radius)*4;
      for (x=radius; x<outw+radius; x++)
        {
          gfloat *pixel= src_buf + src_offset;
          gfloat  min[4];
          gfloat  max[4];

          compute_envelopes (src_buf,
                             inw, inh,
                             x, y,
                             radius, samples,
                             iterations,
                             FALSE, /* same spray */
                             rgamma,
                             min, max);
          {
            /* this should be replaced with a better/faster projection of
             * pixel onto the vector spanned by min -> max, currently
             * computed by comparing the distance to min with the sum
             * of the distance to min/max.
             */

            gfloat nominator = 0;
            gfloat denominator = 0;
            gint c;
            for (c=0; c<3; c++)
              {
                nominator   += (pixel[c] - min[c]) * (pixel[c] - min[c]);
                denominator += (pixel[c] - max[c]) * (pixel[c] - max[c]);
              }

            nominator = sqrt (nominator);
            denominator = sqrt (denominator);
            denominator = nominator + denominator;

            if (denominator>0.000)
              {
                dst_buf[dst_offset+0] = nominator/denominator;
              }
            else
              {
                /* shouldn't happen */
                dst_buf[dst_offset+0] = 0.5;
              }
            dst_buf[dst_offset+1] = src_buf[src_offset+3];

            src_offset+=4;
            dst_offset+=2;
          }
        }
    }
  gegl_buffer_set (dst, dst_rect, 0, babl_format ("YA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_CHANT_PROPERTIES (operation)->radius);

  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
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

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"#define TRUE true                                                     \n"
"                                                                      \n"
"#define FALSE false                                                   \n"
"#define ANGLE_PRIME 95273                                             \n"
"#define RADIUS_PRIME 29537                                            \n"
"                                                                      \n"
"void sample_min_max(const __global   float4 *src_buf,                 \n"
"                                     int     src_width,               \n"
"                                     int     src_height,              \n"
"                    const __global   float  *radiuses,                \n"
"                    const __global   float  *lut_cos,                 \n"
"                    const __global   float  *lut_sin,                 \n"
"                                     int     x,                       \n"
"                                     int     y,                       \n"
"                                     int     radius,                  \n"
"                                     int     samples,                 \n"
"                                     float4 *min,                     \n"
"                                     float4 *max,                     \n"
"                                     int     j,                       \n"
"                                     int     iterations)              \n"
"{                                                                     \n"
"    float4 best_min;                                                  \n"
"    float4 best_max;                                                  \n"
"    float4 center_pix = *(src_buf + src_width * y + x);               \n"
"    int i;                                                            \n"
"                                                                      \n"
"    best_min = center_pix;                                            \n"
"    best_max = center_pix;                                            \n"
"                                                                      \n"
"    int angle_no  = (src_width * y + x) * (iterations) *              \n"
"                       samples + j * samples;                         \n"
"    int radius_no = angle_no;                                         \n"
"    angle_no  %= ANGLE_PRIME;                                         \n"
"    radius_no %= RADIUS_PRIME;                                        \n"
"    for(i=0; i<samples; i++)                                          \n"
"    {                                                                 \n"
"        int angle;                                                    \n"
"        float rmag;                                                   \n"
"        /* if we've sampled outside the valid image                   \n"
"           area, we grab another sample instead, this                 \n"
"           should potentially work better than mirroring              \n"
"           or extending the image */                                  \n"
"                                                                      \n"
"         angle = angle_no++;                                          \n"
"         rmag  = radiuses[radius_no++] * radius;                      \n"
"                                                                      \n"
"         if( angle_no  >= ANGLE_PRIME)                                \n"
"             angle_no   = 0;                                          \n"
"         if( radius_no >= RADIUS_PRIME)                               \n"
"             radius_no  = 0;                                          \n"
"                                                                      \n"
"         int u = x + rmag * lut_cos[angle];                           \n"
"         int v = y + rmag * lut_sin[angle];                           \n"
"                                                                      \n"
"         if(u>=src_width || u <0 || v>=src_height || v<0)             \n"
"         {                                                            \n"
"             //--i;                                                   \n"
"             continue;                                                \n"
"         }                                                            \n"
"         float4 pixel = *(src_buf + (src_width * v + u));             \n"
"         if(pixel.w<=0.0f)                                            \n"
"         {                                                            \n"
"             //--i;                                                   \n"
"             continue;                                                \n"
"         }                                                            \n"
"                                                                      \n"
"         best_min = pixel < best_min ? pixel : best_min;              \n"
"         best_max = pixel > best_max ? pixel : best_max;              \n"
"    }                                                                 \n"
"                                                                      \n"
"    (*min).xyz = best_min.xyz;                                        \n"
"    (*max).xyz = best_max.xyz;                                        \n"
"}                                                                     \n"
"                                                                      \n"
"void compute_envelopes_CL(const __global  float4 *src_buf,            \n"
"                                          int     src_width,          \n"
"                                          int     src_height,         \n"
"                          const __global  float  *radiuses,           \n"
"                          const __global  float  *lut_cos,            \n"
"                          const __global  float  *lut_sin,            \n"
"                                          int     x,                  \n"
"                                          int     y,                  \n"
"                                          int     radius,             \n"
"                                          int     samples,            \n"
"                                          int     iterations,         \n"
"                                          float4 *min_envelope,       \n"
"                                          float4 *max_envelope)       \n"
"{                                                                     \n"
"    float4 range_sum = 0;                                             \n"
"    float4 relative_brightness_sum = 0;                               \n"
"    float4 pixel = *(src_buf + src_width * y + x);                    \n"
"                                                                      \n"
"    int i;                                                            \n"
"    for(i =0; i<iterations; i++)                                      \n"
"    {                                                                 \n"
"        float4 min,max;                                               \n"
"        float4 range, relative_brightness;                            \n"
"                                                                      \n"
"        sample_min_max(src_buf, src_width, src_height,                \n"
"                        radiuses, lut_cos, lut_sin, x, y,             \n"
"                        radius,samples,&min,&max,i,iterations);       \n"
"        range = max - min;                                            \n"
"        relative_brightness = range <= 0.0f ?                         \n"
"                               0.5f : (pixel - min) / range;          \n"
"        relative_brightness_sum += relative_brightness;               \n"
"        range_sum += range;                                           \n"
"    }                                                                 \n"
"                                                                      \n"
"    float4 relative_brightness = relative_brightness_sum / iterations;\n"
"    float4 range = range_sum / iterations;                            \n"
"                                                                      \n"
"    if(max_envelope)                                                  \n"
"        *max_envelope = pixel + (1.0f - relative_brightness) * range; \n"
"                                                                      \n"
"    if(min_envelope)                                                  \n"
"        *min_envelope = pixel - relative_brightness * range;          \n"
"}                                                                     \n"
"                                                                      \n"
"__kernel void C2g_CL(const __global float4 *src_buf,                  \n"
"                                    int     src_width,                \n"
"                                    int     src_height,               \n"
"                     const __global float  *radiuses,                 \n"
"                     const __global float  *lut_cos,                  \n"
"                     const __global float  *lut_sin,                  \n"
"                           __global float2 *dst_buf,                  \n"
"                                    int     radius,                   \n"
"                                    int     samples,                  \n"
"                                    int     iterations)               \n"
"{                                                                     \n"
"    int gidx = get_global_id(0);                                      \n"
"    int gidy = get_global_id(1);                                      \n"
"                                                                      \n"
"    int x = gidx + radius;                                            \n"
"    int y = gidy + radius;                                            \n"
"                                                                      \n"
"    int src_offset = (src_width * y + x);                             \n"
"    int dst_offset = gidx + get_global_size(0) * gidy;                \n"
"    float4 min,max;                                                   \n"
"                                                                      \n"
"    compute_envelopes_CL(src_buf, src_width, src_height,              \n"
"                         radiuses, lut_cos, lut_sin, x, y,            \n"
"                         radius, samples, iterations, &min, &max);    \n"
"                                                                      \n"
"    float4 pixel = *(src_buf + src_offset);                           \n"
"                                                                      \n"
"    float nominator=0, denominator=0;                                 \n"
"    float4 t1 = (pixel - min) * (pixel - min);                        \n"
"    float4 t2 = (pixel - max) * (pixel - max);                        \n"
"                                                                      \n"
"    nominator   = t1.x + t1.y + t1.z;                                 \n"
"    denominator = t2.x + t2.y + t2.z;                                 \n"
"                                                                      \n"
"    nominator   = sqrt(nominator);                                    \n"
"    denominator = sqrt(denominator);                                  \n"
"    denominator+= nominator + denominator;                            \n"
"                                                                      \n"
"    dst_buf[dst_offset].x = (denominator > 0.000f)                    \n"
"                             ? (nominator / denominator) : 0.5f;      \n"
"    dst_buf[dst_offset].y =  src_buf[src_offset].w;                   \n"
"}                                                                     \n"
"                                                                      \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_c2g (cl_mem                in_tex,
    cl_mem                    out_tex,
    size_t                    global_worksize,
    const GeglRectangle      *src_roi,
    const GeglRectangle      *roi,
    gint                      radius,
    gint                      samples,
    gint                      iterations,
    gdouble                   rgamma)
{
  cl_int cl_err = 0;
  cl_mem cl_lut_cos, cl_lut_sin, cl_radiuses;
  const size_t gbl_size[2] = {roi->width, roi->height};

  if (!cl_data)
    {
      const char *kernel_name[] ={"C2g_CL", NULL};
      cl_data = gegl_cl_compile_and_build(kernel_source, kernel_name);
    }
  if (!cl_data)  return 0;

  compute_luts(rgamma);

  cl_lut_cos = gegl_clCreateBuffer(gegl_cl_get_context(),
                                   CL_MEM_READ_ONLY,
                                   ANGLE_PRIME * sizeof(cl_float), NULL, &cl_err);

  cl_err |= gegl_clEnqueueWriteBuffer(gegl_cl_get_command_queue(), cl_lut_cos,
                                      CL_TRUE, 0, ANGLE_PRIME * sizeof(cl_float), lut_cos,
                                      0, NULL, NULL);
  if (CL_SUCCESS != cl_err)   return cl_err;

  cl_lut_sin = gegl_clCreateBuffer(gegl_cl_get_context(),
                                   CL_MEM_READ_ONLY,
                                   ANGLE_PRIME * sizeof(cl_float), NULL, &cl_err);

  cl_err |= gegl_clEnqueueWriteBuffer(gegl_cl_get_command_queue(), cl_lut_sin,
                                      CL_TRUE, 0, ANGLE_PRIME * sizeof(cl_float), lut_sin,
                                      0, NULL, NULL);
  if (CL_SUCCESS != cl_err)    return cl_err;

  cl_radiuses = gegl_clCreateBuffer(gegl_cl_get_context(),
                                    CL_MEM_READ_ONLY,
                                    RADIUS_PRIME * sizeof(cl_float), NULL, &cl_err);

  cl_err |= gegl_clEnqueueWriteBuffer(gegl_cl_get_command_queue(), cl_radiuses,
                                      CL_TRUE, 0, RADIUS_PRIME * sizeof(cl_float), radiuses,
                                      0, NULL, NULL);
  if (CL_SUCCESS != cl_err)    return cl_err;

  {
  cl_int cl_src_width  = src_roi->width;
  cl_int cl_src_height = src_roi->height;
  cl_int cl_radius     = radius;
  cl_int cl_samples    = samples;
  cl_int cl_iterations = iterations;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_int), (void*)&cl_src_width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int), (void*)&cl_src_height);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_mem), (void*)&cl_radiuses);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_mem), (void*)&cl_lut_cos);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_mem), (void*)&cl_lut_sin);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_mem), (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_int), (void*)&cl_radius);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 8, sizeof(cl_int), (void*)&cl_samples);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 9, sizeof(cl_int), (void*)&cl_iterations);
  if (cl_err != CL_SUCCESS) return cl_err;
  }

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(), cl_data->kernel[0],
                                       2, NULL, gbl_size, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
  if (CL_SUCCESS != cl_err) return cl_err;

  gegl_clFinish(gegl_cl_get_command_queue ());

  gegl_clReleaseMemObject(cl_radiuses);
  gegl_clReleaseMemObject(cl_lut_cos);
  gegl_clReleaseMemObject(cl_lut_sin);

  return cl_err;
}

static gboolean
cl_process (GeglOperation       *operation,
      GeglBuffer          *input,
      GeglBuffer          *output,
      const GeglRectangle *result)
{
  const Babl *in_format  = babl_format("RGBA float");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;
  gint j;
  cl_int cl_err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format, GEGL_CL_BUFFER_READ,
                                                           op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_c2g(i->tex[read][j], i->tex[0][j],i->size[0][j], &i->roi[read][j],&i->roi[0][j],
                      o->radius,o->samples,o->iterations,RGAMMA);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:c2g: %s", gegl_cl_errstring(cl_err));
        return FALSE;
      }
    }
  }
  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;
  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (o->radius < 500 && gegl_cl_is_accelerated ())
    if(cl_process(operation, input, output, result))
      return TRUE;

  c2g (input, &compute, output, result,
       o->radius,
       o->samples,
       o->iterations,
       /*o->rgamma*/RGAMMA);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare  = prepare;

  /* we override defined region to avoid growing the size of what is defined
   * by the filter. This also allows the tricks used to treat alpha==0 pixels
   * in the image as source data not to be skipped by the stochastic sampling
   * yielding correct edge behavior.
   */
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:c2g",
      "categories",  "enhance",
      "description", 
     _("Color to grayscale conversion, uses envelopes formed from spatial "
       " color differences to perform color-feature preserving grayscale "
       " spatial contrast enhancement"),
     NULL);
}

#endif
