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

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "edge-laplace.c"

#include "gegl-chant.h"
#include <math.h>

#define LAPLACE_RADIUS 1

static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect);

#include <stdio.h>

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  //GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = LAPLACE_RADIUS;
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglRectangle compute;

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

  compute = gegl_operation_get_required_for_output (operation, "input", result);
  edge_laplace (input, &compute, output, result);

  return  TRUE;
}

static void
minmax  (gfloat  x1,
         gfloat  x2,
         gfloat  x3,
         gfloat  x4,
         gfloat  x5,
         gfloat *min_result,
         gfloat *max_result)
{
  gfloat min1, min2, max1, max2;

  if (x1 > x2)
    {
      max1 = x1;
      min1 = x2;
    }
  else
    {
      max1 = x2;
      min1 = x1;
    }

  if (x3 > x4)
    {
      max2 = x3;
      min2 = x4;
    }
  else
    {
      max2 = x4;
      min2 = x3;
    }

  if (min1 < min2)
    *min_result = fminf (min1, x5);
  else
    *min_result = fminf (min2, x5);

  if (max1 > max2)
    *max_result = fmaxf (max1, x5);
  else
    *max_result = fmaxf (max2, x5);
}


static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect)
{

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *temp_buf;
  gfloat *dst_buf;

  gint src_width = src_rect->width;

  src_buf  = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  temp_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf  = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0,
                   babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {
        gfloat *src_pix;

        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gint c;

        gfloat minval, maxval;

        gint i=x+LAPLACE_RADIUS, j=y+LAPLACE_RADIUS;
        offset = i + j * src_width;
        src_pix = src_buf + offset * 4;

        for (c=0;c<3;c++)
          {
            minmax (src_pix[c-src_width*4], src_pix[c+src_width*4],
                    src_pix[c-4], src_pix[c+4], src_pix[c],
                    &minval, &maxval); /* four-neighbourhood */

            gradient[c] = 0.5f * fmaxf((maxval-src_pix[c]), (src_pix[c]-minval));

            gradient[c] = (src_pix[c-4-src_width*4] +
                           src_pix[c-src_width*4] +
                           src_pix[c+4-src_width*4] +

                           src_pix[c-4] -8.0f* src_pix[c] +src_pix[c+4] +

                           src_pix[c-4+src_width*4] + src_pix[c+src_width*4] +
                           src_pix[c+4+src_width*4]) > 0.0f?
                          gradient[c] : -1.0f*gradient[c];
        }

        //alpha
        gradient[3] = src_pix[3];

        for (c=0; c<4;c++)
          temp_buf[offset*4+c] = gradient[c];
      }

  //1-pixel edges
  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {

        gfloat value[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gint c;

        gint i=x+LAPLACE_RADIUS, j=y+LAPLACE_RADIUS;
        gfloat *src_pix = temp_buf + (i + j * src_width) * 4;

        for (c=0;c<3;c++)
        {
          gfloat current = src_pix[c];
          current = ((current > 0.0f) &&
                     (src_pix[c-4-src_width*4] < 0.0f ||
                      src_pix[c+4-src_width*4] < 0.0f ||
                      src_pix[c  -src_width*4] < 0.0f ||
                      src_pix[c-4+src_width*4] < 0.0f ||
                      src_pix[c+4+src_width*4] < 0.0f ||
                      src_pix[   +src_width*4] < 0.0f ||
                      src_pix[c-4            ] < 0.0f ||
                      src_pix[c+4            ] < 0.0f))?
                    current : 0.0f;

          value[c] = current;
        }

        //alpha
        value[3] = src_pix[3];

        for (c=0; c<4;c++)
          dst_buf[offset*4+c] = value[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (temp_buf);
  g_free (dst_buf);
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"#define LAPLACE_RADIUS 1                                              \n"
"void minmax(float x1, float x2, float x3,                             \n"
"            float x4, float x5,                                       \n"
"            float *min_result,                                        \n"
"            float *max_result)                                        \n"
"{                                                                     \n"
"    float min1, min2, max1, max2;                                     \n"
"                                                                      \n"
"    if (x1 > x2)                                                      \n"
"    {                                                                 \n"
"        max1 = x1;                                                    \n"
"        min1 = x2;                                                    \n"
"    }                                                                 \n"
"    else                                                              \n"
"    {                                                                 \n"
"        max1 = x2;                                                    \n"
"        min1 = x1;                                                    \n"
"    }                                                                 \n"
"                                                                      \n"
"    if (x3 > x4)                                                      \n"
"    {                                                                 \n"
"        max2 = x3;                                                    \n"
"        min2 = x4;                                                    \n"
"    }                                                                 \n"
"    else                                                              \n"
"    {                                                                 \n"
"        max2 = x4;                                                    \n"
"        min2 = x3;                                                    \n"
"    }                                                                 \n"
"                                                                      \n"
"    if (min1 < min2)                                                  \n"
"        *min_result = fmin(min1, x5);                                 \n"
"    else                                                              \n"
"        *min_result = fmin(min2, x5);                                 \n"
"    if (max1 > max2)                                                  \n"
"        *max_result = fmax(max1, x5);                                 \n"
"    else                                                              \n"
"        *max_result = fmax(max2, x5);                                 \n"
"}                                                                     \n"
"                                                                      \n"
"kernel void pre_edgelaplace (global float4 *in,                       \n"
"                             global float4 *out)                      \n"
"{                                                                     \n"
"    int gidx = get_global_id(0);                                      \n"
"    int gidy = get_global_id(1);                                      \n"
"                                                                      \n"
"    int src_width  = get_global_size(0) + LAPLACE_RADIUS * 2;         \n"
"    int src_height = get_global_size(1);                              \n"
"                                                                      \n"
"    int i = gidx + LAPLACE_RADIUS, j = gidy + LAPLACE_RADIUS;         \n"
"    int gid1d = i + j * src_width;                                    \n"
"                                                                      \n"
"    float pix_fl[4] = {                                               \n"
"        in[gid1d - 1 - src_width].x, in[gid1d - 1 - src_width].y,     \n"
"        in[gid1d - 1 - src_width].z, in[gid1d - 1 - src_width].w      \n"
"    };                                                                \n"
"    float pix_fm[4] = {                                               \n"
"        in[gid1d     - src_width].x, in[gid1d     - src_width].y,     \n"
"        in[gid1d     - src_width].z, in[gid1d     - src_width].w      \n"
"    };                                                                \n"
"    float pix_fr[4] = {                                               \n"
"        in[gid1d + 1 - src_width].x, in[gid1d + 1 - src_width].y,     \n"
"        in[gid1d + 1 - src_width].z, in[gid1d + 1 - src_width].w      \n"
"    };                                                                \n"
"    float pix_ml[4] = {                                               \n"
"        in[gid1d - 1            ].x, in[gid1d - 1            ].y,     \n"
"        in[gid1d - 1            ].z, in[gid1d - 1            ].w      \n"
"    };                                                                \n"
"    float pix_mm[4] = {                                               \n"
"        in[gid1d                ].x, in[gid1d                ].y,     \n"
"        in[gid1d                ].z, in[gid1d                ].w      \n"
"    };                                                                \n"
"    float pix_mr[4] = {                                               \n"
"        in[gid1d + 1            ].x, in[gid1d + 1            ].y,     \n"
"        in[gid1d + 1            ].z, in[gid1d + 1            ].w      \n"
"    };                                                                \n"
"    float pix_bl[4] = {                                               \n"
"        in[gid1d - 1 + src_width].x, in[gid1d - 1 + src_width].y,     \n"
"        in[gid1d - 1 + src_width].z, in[gid1d - 1 + src_width].w      \n"
"    };                                                                \n"
"    float pix_bm[4] = {                                               \n"
"        in[gid1d     + src_width].x, in[gid1d     + src_width].y,     \n"
"        in[gid1d     + src_width].z, in[gid1d     + src_width].w      \n"
"    };                                                                \n"
"    float pix_br[4] = {                                               \n"
"        in[gid1d + 1 + src_width].x, in[gid1d + 1 + src_width].y,     \n"
"        in[gid1d + 1 + src_width].z, in[gid1d + 1 + src_width].w      \n"
"    };                                                                \n"
"                                                                      \n"
"    int c;                                                            \n"
"    float minval, maxval;                                             \n"
"    float gradient[4];                                                \n"
"                                                                      \n"
"    for (c = 0;c < 3; ++c)                                            \n"
"    {                                                                 \n"
"        minmax(pix_fm[c], pix_bm[c], pix_ml[c], pix_mr[c],            \n"
"            pix_mm[c], &minval, &maxval);                             \n"
"        gradient[c] = 0.5f *                                          \n"
"            fmax((maxval - pix_mm[c]),(pix_mm[c] - minval));          \n"
"        gradient[c] =                                                 \n"
"            (pix_fl[c] + pix_fm[c] + pix_fr[c] +                      \n"
"             pix_ml[c] + pix_mr[c] + pix_bl[c] +                      \n"
"             pix_bm[c] + pix_br[c] - 8.0f * pix_mm[c]) >              \n"
"             0.0f ? gradient[c] : -1.0f * gradient[c];                \n"
"    }                                                                 \n"
"    gradient[3] = pix_mm[3];                                          \n"
"                                                                      \n"
"    out[gid1d] = (float4)                                             \n"
"        (gradient[0], gradient[1], gradient[2], gradient[3]);         \n"
"}                                                                     \n"
"                                                                      \n"
"kernel void knl_edgelaplace (global float4 *in,                       \n"
"                             global float4 *out)                      \n"
"{                                                                     \n"
"    int gidx = get_global_id(0);                                      \n"
"    int gidy = get_global_id(1);                                      \n"
"                                                                      \n"
"    int src_width  = get_global_size(0) + LAPLACE_RADIUS * 2;         \n"
"    int src_height = get_global_size(1);                              \n"
"                                                                      \n"
"    int i = gidx + LAPLACE_RADIUS, j = gidy + LAPLACE_RADIUS;         \n"
"    int gid1d = i + j * src_width;                                    \n"
"                                                                      \n"
"    float pix_fl[4] = {                                               \n"
"        in[gid1d - 1 - src_width].x, in[gid1d - 1 - src_width].y,     \n"
"        in[gid1d - 1 - src_width].z, in[gid1d - 1 - src_width].w      \n"
"    };                                                                \n"
"    float pix_fm[4] = {                                               \n"
"        in[gid1d     - src_width].x, in[gid1d     - src_width].y,     \n"
"        in[gid1d     - src_width].z, in[gid1d     - src_width].w      \n"
"    };                                                                \n"
"    float pix_fr[4] = {                                               \n"
"        in[gid1d + 1 - src_width].x, in[gid1d + 1 - src_width].y,     \n"
"        in[gid1d + 1 - src_width].z, in[gid1d + 1 - src_width].w      \n"
"    };                                                                \n"
"    float pix_ml[4] = {                                               \n"
"        in[gid1d - 1            ].x, in[gid1d - 1            ].y,     \n"
"        in[gid1d - 1            ].z, in[gid1d - 1            ].w      \n"
"    };                                                                \n"
"    float pix_mm[4] = {                                               \n"
"        in[gid1d                ].x, in[gid1d                ].y,     \n"
"        in[gid1d                ].z, in[gid1d                ].w      \n"
"    };                                                                \n"
"    float pix_mr[4] = {                                               \n"
"        in[gid1d + 1            ].x, in[gid1d + 1            ].y,     \n"
"        in[gid1d + 1            ].z, in[gid1d + 1            ].w      \n"
"    };                                                                \n"
"    float pix_bl[4] = {                                               \n"
"        in[gid1d - 1 + src_width].x, in[gid1d - 1 + src_width].y,     \n"
"        in[gid1d - 1 + src_width].z, in[gid1d - 1 + src_width].w      \n"
"    };                                                                \n"
"    float pix_bm[4] = {                                               \n"
"        in[gid1d     + src_width].x, in[gid1d     + src_width].y,     \n"
"        in[gid1d     + src_width].z, in[gid1d     + src_width].w      \n"
"    };                                                                \n"
"    float pix_br[4] = {                                               \n"
"        in[gid1d + 1 + src_width].x, in[gid1d + 1 + src_width].y,     \n"
"        in[gid1d + 1 + src_width].z, in[gid1d + 1 + src_width].w      \n"
"    };                                                                \n"
"                                                                      \n"
"    int c;                                                            \n"
"    float value[4];                                                   \n"
"                                                                      \n"
"    for (c = 0;c < 3; ++c)                                            \n"
"    {                                                                 \n"
"        float current = pix_mm[c];                                    \n"
"        current =                                                     \n"
"            ((current > 0.0f) &&                                      \n"
"             (pix_fl[c] < 0.0f || pix_fm[c] < 0.0f ||                 \n"
"              pix_fr[c] < 0.0f || pix_ml[c] < 0.0f ||                 \n"
"              pix_mr[c] < 0.0f || pix_bl[c] < 0.0f ||                 \n"
"              pix_bm[c] < 0.0f || pix_br[c] < 0.0f )                  \n"
"            ) ? current : 0.0f;                                       \n"
"        value[c] = current;                                           \n"
"    }                                                                 \n"
"    value[3] = pix_mm[3];                                             \n"
"                                                                      \n"
"    out[gidx + gidy * get_global_size(0)] = (float4)                  \n"
"        (value[0], value[1], value[2], value[3]);                     \n"
"}                                                                     \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_edge_laplace (cl_mem                in_tex,
                 cl_mem                aux_tex,
                 cl_mem                out_tex,
                 const GeglRectangle  *src_rect,
                 const GeglRectangle  *roi,
                 gint                  radius)
{
  cl_int cl_err = 0;
  size_t global_ws[2];
  if (!cl_data)
  {
    const char *kernel_name[] = {"pre_edgelaplace", "knl_edgelaplace", NULL};
    cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
  }

  if (!cl_data) return 1;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
  if (CL_SUCCESS != cl_err) return cl_err;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),   (void*)&aux_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),   (void*)&out_tex);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[1], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;
  return cl_err;
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

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,   result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format,  GEGL_CL_BUFFER_READ, op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  gint aux  = gegl_buffer_cl_iterator_add_2 (i, NULL, result, in_format,  GEGL_CL_BUFFER_AUX, op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_edge_laplace(i->tex[read][j], i->tex[aux][j], i->tex[0][j], &i->roi[read][j], &i->roi[0][j], LAPLACE_RADIUS);
      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:edge-laplace: %s", gegl_cl_errstring(cl_err));
        return FALSE;
      }
    }
  }
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:edge-laplace",
    "categories"  , "edge-detect",
    "description" ,
          _("High-resolution edge detection"),
    NULL);
}

#endif
