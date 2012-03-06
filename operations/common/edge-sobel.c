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
 */

/*
 * Copyright 2011 Victor Oliveira <victormatheus@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_boolean (horizontal,  _("Horizontal"),  TRUE,
                    _("Horizontal"))

gegl_chant_boolean (vertical,  _("Vertical"),  TRUE,
                    _("Vertical"))

gegl_chant_boolean (keep_signal,  _("Keep Signal"),  TRUE,
                    _("Keep Signal"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "edge-sobel.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

#define SOBEL_RADIUS 1

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal);



static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  //GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = SOBEL_RADIUS;
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"#define SOBEL_RADIUS 1                                                \n"
"kernel void kernel_edgesobel(global float4 *in,                       \n"
"                             global float4 *out,                      \n"
"                             const int horizontal,                    \n"
"                             const int vertical,                      \n"
"                             const int keep_signal)                   \n"
"{                                                                     \n"
"    int gidx = get_global_id(0);                                      \n"
"    int gidy = get_global_id(1);                                      \n"
"                                                                      \n"
"    float4 hor_grad = 0.0f;                                           \n"
"    float4 ver_grad = 0.0f;                                           \n"
"    float4 gradient = 0.0f;                                           \n"
"                                                                      \n"
"    int dst_width = get_global_size(0);                               \n"
"    int src_width = dst_width + SOBEL_RADIUS * 2;                     \n"
"                                                                      \n"
"    int i = gidx + SOBEL_RADIUS, j = gidy + SOBEL_RADIUS;             \n"
"    int gid1d = i + j * src_width;                                    \n"
"                                                                      \n"
"    float4 pix_fl = in[gid1d - 1 - src_width];                        \n"
"    float4 pix_fm = in[gid1d     - src_width];                        \n"
"    float4 pix_fr = in[gid1d + 1 - src_width];                        \n"
"    float4 pix_ml = in[gid1d - 1            ];                        \n"
"    float4 pix_mm = in[gid1d                ];                        \n"
"    float4 pix_mr = in[gid1d + 1            ];                        \n"
"    float4 pix_bl = in[gid1d - 1 + src_width];                        \n"
"    float4 pix_bm = in[gid1d     + src_width];                        \n"
"    float4 pix_br = in[gid1d + 1 + src_width];                        \n"
"                                                                      \n"
"    if (horizontal)                                                   \n"
"    {                                                                 \n"
"        hor_grad +=                                                   \n"
"            - 1.0f * pix_fl + 1.0f * pix_fr                           \n"
"            - 2.0f * pix_ml + 2.0f * pix_mr                           \n"
"            - 1.0f * pix_bl + 1.0f * pix_br;                          \n"
"    }                                                                 \n"
"    if (vertical)                                                     \n"
"    {                                                                 \n"
"        ver_grad +=                                                   \n"
"            - 1.0f * pix_fl - 2.0f * pix_fm                           \n"
"            - 1.0f * pix_fr + 1.0f * pix_bl                           \n"
"            + 2.0f * pix_bm + 1.0f * pix_br;                          \n"
"    }                                                                 \n"
"                                                                      \n"
"    if (horizontal && vertical)                                       \n"
"    {                                                                 \n"
"        gradient = sqrt(                                              \n"
"            hor_grad * hor_grad +                                     \n"
"            ver_grad * ver_grad) / 1.41f;                             \n"
"    }                                                                 \n"
"    else                                                              \n"
"    {                                                                 \n"
"        if (keep_signal)                                              \n"
"            gradient = hor_grad + ver_grad;                           \n"
"        else                                                          \n"
"            gradient = fabs(hor_grad + ver_grad);                     \n"
"    }                                                                 \n"
"                                                                      \n"
"    gradient.w = pix_mm.w;                                            \n"
"                                                                      \n"
"    out[gidx + gidy * dst_width] = gradient;                          \n"
"}                                                                     \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_edge_sobel (cl_mem              in_tex,
               cl_mem              out_tex,
               size_t              global_worksize,
               const GeglRectangle *roi,
               gboolean            horizontal,
               gboolean            vertical,
               gboolean            keep_signal)
{
  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_edgesobel", NULL};
      cl_data = gegl_cl_compile_and_build(kernel_source, kernel_name);
    }
  if (!cl_data)  return 0;

  {
  const size_t gbl_size[2] = {roi->width,roi->height};
  cl_int n_horizontal  = horizontal;
  cl_int n_vertical    = vertical;
  cl_int n_keep_signal = keep_signal;
  cl_int cl_err = 0;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem), (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int), (void*)&n_horizontal);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int), (void*)&n_vertical);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int), (void*)&n_keep_signal);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(),
                                       cl_data->kernel[0], 2,
                                       NULL, gbl_size, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;
  }

  return CL_SUCCESS;
}

static gboolean
cl_process (GeglOperation       *operation,
      GeglBuffer          *input,
      GeglBuffer          *output,
      const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;
  gint j;
  cl_int cl_err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
                gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format, GEGL_CL_BUFFER_READ,op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_edge_sobel(i->tex[read][j], i->tex[0][j], i->size[0][j],&i->roi[0][j], o->horizontal, o->vertical, o->keep_signal);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:edge-sobel: %s", gegl_cl_errstring(cl_err));
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
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;

  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (gegl_cl_is_accelerated ())
    if(cl_process(operation, input, output, result))
      return TRUE;

  edge_sobel (input, &compute, output, result, o->horizontal, o->vertical, o->keep_signal);
  return  TRUE;
}

inline static gfloat
RMS(gfloat a, gfloat b)
{
  return sqrtf(a*a+b*b);
}

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal)
{

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  gint src_width = src_rect->width;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {

        gfloat hor_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat ver_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gfloat *center_pix = src_buf + ((x+SOBEL_RADIUS)+((y+SOBEL_RADIUS) * src_width)) * 4;

        gint c;

        if (horizontal)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                hor_grad[c] +=
                    -1.0f*src_pix[c-4-src_width*4]+ src_pix[c+4-src_width*4] +
                    -2.0f*src_pix[c-4] + 2.0f*src_pix[c+4] +
                    -1.0f*src_pix[c-4+src_width*4]+ src_pix[c+4+src_width*4];
          }

        if (vertical)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                ver_grad[c] +=
                  -1.0f*src_pix[c-4-src_width*4]-2.0f*src_pix[c-src_width*4]-1.0f*src_pix[c+4-src_width*4] +
                  src_pix[c-4+src_width*4]+2.0f*src_pix[c+src_width*4]+     src_pix[c+4+src_width*4];
        }

        if (horizontal && vertical)
          {
            for (c=0;c<3;c++)
              // normalization to [0, 1]
              gradient[c] = RMS(hor_grad[c],ver_grad[c])/1.41f;
          }
        else
          {
            if (keep_signal)
              {
                for (c=0;c<3;c++)
                  gradient[c] = hor_grad[c]+ver_grad[c];
              }
            else
              {
                for (c=0;c<3;c++)
                  gradient[c] = fabsf(hor_grad[c]+ver_grad[c]);
              }
          }

        //alpha
        gradient[3] = center_pix[3];

        for (c=0; c<4;c++)
          dst_buf[offset*4+c] = gradient[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:edge-sobel",
    "categories" , "edge-detect",
    "description",
          _("Specialized direction-dependent edge detection"),
          NULL);
}

#endif
