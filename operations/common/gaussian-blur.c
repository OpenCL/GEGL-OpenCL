/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Dominik Ernst <dernst@gmx.de>
 *
 * Recursive Gauss IIR Filter as described by Young / van Vliet
 * in "Signal Processing 44 (1995) 139 - 151"
 *
 * NOTE: The IIR filter should not be used for radius < 0.5, since it
 *       becomes very inaccurate.
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (std_dev_x, _("Size X"), 0.0, 10000.0, 4.0, 0.0, 1000.0, 1.5,
   _("Standard deviation for the horizontal axis. (multiply by ~2 to get radius)"))
gegl_chant_double_ui (std_dev_y, _("Size Y"), 0.0, 10000.0, 4.0, 0.0, 1000.0, 1.5,
   _("Standard deviation for the vertical axis. (multiply by ~2 to get radius.)"))
gegl_chant_string (filter, _("Filter"), "auto",
   _("Optional parameter to override the automatic selection of blur filter. "
     "Choices are fir, iir, auto"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "gaussian-blur.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

#define RADIUS_SCALE   4


static void
iir_young_find_constants (gfloat   radius,
                          gdouble *B,
                          gdouble *b);

static gint
fir_gen_convolve_matrix (gdouble   sigma,
                         gdouble **cmatrix_p);


static void
iir_young_find_constants (gfloat   sigma,
                          gdouble *B,
                          gdouble *b)
{
  gdouble q;

  if (sigma == 0.0)
    {
      /* to prevent unexpected ringing at tile boundaries,
         we define an (expensive) copy operation here */
      *B = 1.0;
      b[0] = 1.0;
      b[1] = b[2] = b[3] = 0.0;
      return;
    }

  if(sigma >= 2.5)
    q = 0.98711*sigma - 0.96330;
  else
    q = 3.97156 - 4.14554*sqrt(1-0.26891*sigma);

  b[0] = 1.57825 + (2.44413*q) + (1.4281*q*q) + (0.422205*q*q*q);
  b[1] = (2.44413*q) + (2.85619*q*q) + (1.26661*q*q*q);
  b[2] = -((1.4281*q*q) + (1.26661*q*q*q));
  b[3] = 0.422205*q*q*q;

  *B = 1 - ( (b[1]+b[2]+b[3])/b[0] );
}

static inline void
iir_young_blur_1D (gfloat  * buf,
                   gint      offset,
                   gint      delta_offset,
                   gdouble   B,
                   gdouble * b,
                   gfloat  * w,
                   gint      w_len)
{
  gint wcount, i;
  gdouble tmp;
  gdouble recip = 1.0 / b[0];

  /* forward filter */
  wcount = 0;

  while (wcount < w_len)
    {
      tmp = 0;

      for (i=1; i<4; i++)
        {
          if (wcount-i >= 0)
            tmp += b[i]*w[wcount-i];
        }

      tmp *= recip;
      tmp += B*buf[offset];
      w[wcount] = tmp;

      wcount++;
      offset += delta_offset;
    }

  /* backward filter */
  wcount = w_len - 1;
  offset -= delta_offset;

  while (wcount >= 0)
    {
      tmp = 0;

      for (i=1; i<4; i++)
        {
          if (wcount+i < w_len)
            tmp += b[i]*buf[offset+delta_offset*i];
        }

      tmp *= recip;
      tmp += B*w[wcount];
      buf[offset] = tmp;

      offset -= delta_offset;
      wcount--;
    }
}

/* expects src and dst buf to have the same height and no y-offset */
static void
iir_young_hor_blur (GeglBuffer          *src,
                    const GeglRectangle *src_rect,
                    GeglBuffer          *dst,
                    const GeglRectangle *dst_rect,
                    gdouble              B,
                    gdouble             *b)
{
  gint v;
  gint c;
  gint w_len;
  gfloat *buf;
  gfloat *w;

  buf = g_new0 (gfloat, src_rect->height * src_rect->width * 4);
  w   = g_new0 (gfloat, src_rect->width);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  w_len = src_rect->width;
  for (v=0; v<src_rect->height; v++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             v*src_rect->width*4+c,
                             4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set (dst, src_rect, 0.0, babl_format ("RaGaBaA float"),
                   buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
  g_free (w);
}

/* expects src and dst buf to have the same width and no x-offset */
static void
iir_young_ver_blur (GeglBuffer          *src,
                    const GeglRectangle *src_rect,
                    GeglBuffer          *dst,
                    const GeglRectangle *dst_rect,
                    gdouble              B,
                    gdouble             *b)
{
  gint u;
  gint c;
  gint w_len;
  gfloat *buf;
  gfloat *w;

  buf = g_new0 (gfloat, src_rect->height * src_rect->width * 4);
  w   = g_new0 (gfloat, src_rect->height);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  w_len = src_rect->height;
  for (u=0; u<dst_rect->width; u++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             u*4 + c,
                             src_rect->width*4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set (dst, src_rect, 0,
                   babl_format ("RaGaBaA float"), buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
  g_free (w);
}


static gint
fir_calc_convolve_matrix_length (gdouble sigma)
{
  return sigma ? ceil (sigma)*6.0+1.0 : 1;
}

static gint
fir_gen_convolve_matrix (gdouble   sigma,
                         gdouble **cmatrix_p)
{
  gint     matrix_length;
  gdouble *cmatrix;

  matrix_length = fir_calc_convolve_matrix_length (sigma);
  cmatrix = g_new (gdouble, matrix_length);
  if (!cmatrix)
    return 0;

  if (matrix_length == 1)
    {
      cmatrix[0] = 1;
    }
  else
    {
      gint i,x;
      gdouble sum = 0;

      for (i=0; i<matrix_length/2+1; i++)
        {
          gdouble y;

          x = i - (matrix_length/2);
          y = (1.0/(sigma*sqrt(2.0*G_PI))) * exp(-(x*x) / (2.0*sigma*sigma));

          cmatrix[i] = y;
          sum += cmatrix[i];
        }

      for (i=matrix_length/2 + 1; i<matrix_length; i++)
        {
          cmatrix[i] = cmatrix[matrix_length-i-1];
          sum += cmatrix[i];
        }

      for (i=0; i<matrix_length; i++)
      {
        cmatrix[i] /= sum;
      }
    }

  *cmatrix_p = cmatrix;
  return matrix_length;
}

static inline float
fir_get_mean_component_1D (gfloat  * buf,
                           gint      offset,
                           gint      delta_offset,
                           gdouble * cmatrix,
                           gint      matrix_length)
{
  gint    i;
  gdouble acc=0;

  for (i=0; i < matrix_length; i++)
    {
      acc += buf[offset] * cmatrix[i];
      offset += delta_offset;
    }

  return acc;
}

/* expects src and dst buf to have the same height and no y-offset */
static void
fir_hor_blur (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length,
              gint                 xoff) /* offset between src and dst */
{
  gint        u,v;
  gint        offset;
  gfloat     *src_buf;
  gfloat     *dst_buf;
  const gint  radius = matrix_length/2;
  const gint  src_width = src_rect->width;

  g_assert (xoff >= radius);

  src_buf = g_new0 (gfloat, src_rect->height * src_rect->width * 4);
  dst_buf = g_new0 (gfloat, dst_rect->height * dst_rect->width * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset = 0;
  for (v=0; v<dst_rect->height; v++)
    for (u=0; u<dst_rect->width; u++)
      {
        gint src_offset = (u-radius+xoff + v*src_width) * 4;
        gint c;
        for (c=0; c<4; c++)
          dst_buf [offset++] = fir_get_mean_component_1D (src_buf,
                                                          src_offset + c,
                                                          4,
                                                          cmatrix,
                                                          matrix_length);
      }

  gegl_buffer_set (dst, dst_rect, 0.0, babl_format ("RaGaBaA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

/* expects src and dst buf to have the same width and no x-offset */
static void
fir_ver_blur (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length,
              gint                 yoff) /* offset between src and dst */
{
  gint        u,v;
  gint        offset;
  gfloat     *src_buf;
  gfloat     *dst_buf;
  const gint  radius = matrix_length/2;
  const gint  src_width = src_rect->width;

  g_assert (yoff >= radius);

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("RaGaBaA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset=0;
  for (v=0; v< dst_rect->height; v++)
    for (u=0; u< dst_rect->width; u++)
      {
        gint src_offset = (u + (v-radius+yoff)*src_width) * 4;
        gint c;
        for (c=0; c<4; c++)
          dst_buf [offset++] = fir_get_mean_component_1D (src_buf,
                                                          src_offset + c,
                                                          src_width * 4,
                                                          cmatrix,
                                                          matrix_length);
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RaGaBaA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
#define max(A,B) ((A) > (B) ? (A) : (B))
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o    = GEGL_CHANT_PROPERTIES (operation);

  gfloat fir_radius_x = fir_calc_convolve_matrix_length (o->std_dev_x) / 2;
  gfloat fir_radius_y = fir_calc_convolve_matrix_length (o->std_dev_y) / 2;
  gfloat iir_radius_x = o->std_dev_x * RADIUS_SCALE;
  gfloat iir_radius_y = o->std_dev_y * RADIUS_SCALE;

  /* XXX: these should be calculated exactly considering o->filter, but we just
   * make sure there is enough space */
  area->left = area->right = ceil ( max (fir_radius_x, iir_radius_x));
  area->top = area->bottom = ceil ( max (fir_radius_y, iir_radius_y));

  gegl_operation_set_format (operation, "input",
                             babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RaGaBaA float"));
#undef max
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"float4 fir_get_mean_component_1D_CL(const global float4 *buf,     \n"
"                                    int offset,                   \n"
"                                    const int delta_offset,       \n"
"                                    constant float *cmatrix,      \n"
"                                    const int matrix_length)      \n"
"{                                                                 \n"
"    float4 acc = 0.0f;                                            \n"
"    int i;                                                        \n"
"                                                                  \n"
"    for(i=0; i<matrix_length; i++)                                \n"
"      {                                                           \n"
"        acc    += buf[offset] * cmatrix[i];                       \n"
"        offset += delta_offset;                                   \n"
"      }                                                           \n"
"    return acc;                                                   \n"
"}                                                                 \n"
"                                                                  \n"
"__kernel void fir_ver_blur_CL(const global float4 *src_buf,       \n"
"                              const int src_width,                \n"
"                              global float4 *dst_buf,             \n"
"                              constant float *cmatrix,            \n"
"                              const int matrix_length,            \n"
"                              const int yoff)                     \n"
"{                                                                 \n"
"    int gidx = get_global_id(0);                                  \n"
"    int gidy = get_global_id(1);                                  \n"
"    int gid  = gidx + gidy * get_global_size(0);                  \n"
"                                                                  \n"
"    int radius = matrix_length / 2;                               \n"
"    int src_offset = gidx + (gidy - radius + yoff) * src_width;   \n"
"                                                                  \n"
"    dst_buf[gid] = fir_get_mean_component_1D_CL(                  \n"
"        src_buf, src_offset, src_width, cmatrix, matrix_length);  \n"
"}                                                                 \n"
"                                                                  \n"
"__kernel void fir_hor_blur_CL(const global float4 *src_buf,       \n"
"                              const int src_width,                \n"
"                              global float4 *dst_buf,             \n"
"                              constant float *cmatrix,            \n"
"                              const int matrix_length,            \n"
"                              const int yoff)                     \n"
"{                                                                 \n"
"    int gidx = get_global_id(0);                                  \n"
"    int gidy = get_global_id(1);                                  \n"
"    int gid  = gidx + gidy * get_global_size(0);                  \n"
"                                                                  \n"
"    int radius = matrix_length / 2;                               \n"
"    int src_offset = gidy * src_width + (gidx - radius + yoff);   \n"
"                                                                  \n"
"    dst_buf[gid] = fir_get_mean_component_1D_CL(                  \n"
"        src_buf, src_offset, 1, cmatrix, matrix_length);          \n"
"}                                                                 \n";

static gegl_cl_run_data *cl_data = NULL;

static cl_int
cl_gaussian_blur (cl_mem                in_tex,
                  cl_mem                out_tex,
                  cl_mem                aux_tex,
                  size_t                global_worksize,
                  const GeglRectangle  *roi,
                  const GeglRectangle  *src_rect,
                  const GeglRectangle  *aux_rect,
                  gfloat               *dmatrix_x,
                  gint                  matrix_length_x,
                  gint                  xoff,
                  gfloat               *dmatrix_y,
                  gint                  matrix_length_y,
                  gint                  yoff)
{
  cl_int cl_err = 0;

  size_t global_ws[2];

  cl_mem cl_matrix_x, cl_matrix_y;

  if (!cl_data)
    {
      const char *kernel_name[] = {"fir_ver_blur_CL", "fir_hor_blur_CL", NULL};
      cl_data = gegl_cl_compile_and_build (kernel_source, kernel_name);
    }
  if (!cl_data) return 1;

  cl_matrix_x = gegl_clCreateBuffer(gegl_cl_get_context(),
                                    CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                    matrix_length_x * sizeof(cl_float), NULL, &cl_err);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueWriteBuffer(gegl_cl_get_command_queue(), cl_matrix_x,
                                     CL_TRUE, 0, matrix_length_x * sizeof(cl_float), dmatrix_x,
                                     0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_matrix_y = gegl_clCreateBuffer(gegl_cl_get_context(),
                                    CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,
                                    matrix_length_y * sizeof(cl_float), NULL, &cl_err);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueWriteBuffer(gegl_cl_get_command_queue(), cl_matrix_y,
                                     CL_TRUE, 0, matrix_length_y * sizeof(cl_float), dmatrix_y,
                                     0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  {
  global_ws[0] = aux_rect->width;
  global_ws[1] = aux_rect->height;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem), (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_int), (void*)&src_rect->width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_mem), (void*)&aux_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 3, sizeof(cl_mem), (void*)&cl_matrix_x);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 4, sizeof(cl_int), (void*)&matrix_length_x);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[1], 5, sizeof(cl_int), (void*)&xoff);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[1], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;
  }

  {
  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&aux_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_int), (void*)&aux_rect->width);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem), (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_mem), (void*)&cl_matrix_y);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int), (void*)&matrix_length_y);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_int), (void*)&yoff);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;
  }

  gegl_clFinish(gegl_cl_get_command_queue ());

  gegl_clReleaseMemObject(cl_matrix_x);
  gegl_clReleaseMemObject(cl_matrix_y);

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

  gdouble *cmatrix_x, *cmatrix_y;
  gint cmatrix_len_x, cmatrix_len_y;

  gfloat *fmatrix_x, *fmatrix_y;

  cmatrix_len_x = fir_gen_convolve_matrix (o->std_dev_x, &cmatrix_x);
  cmatrix_len_y = fir_gen_convolve_matrix (o->std_dev_y, &cmatrix_y);

  fmatrix_x = g_new (gfloat, cmatrix_len_x);
  fmatrix_y = g_new (gfloat, cmatrix_len_y);

  for(j=0; j<cmatrix_len_x; j++)
    fmatrix_x[j] = (gfloat) cmatrix_x[j];

  for(j=0; j<cmatrix_len_y; j++)
    fmatrix_y[j] = (gfloat) cmatrix_y[j];

  {
  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  gint read = gegl_buffer_cl_iterator_add_2 (i, input, result, in_format, GEGL_CL_BUFFER_READ,
                                             op_area->left, op_area->right, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);
  gint aux  = gegl_buffer_cl_iterator_add_2 (i, NULL, result, in_format,  GEGL_CL_BUFFER_AUX,
                                             0, 0, op_area->top, op_area->bottom, GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;
      for (j=0; j < i->n; j++)
        {
           cl_err = cl_gaussian_blur(i->tex[read][j],
                                     i->tex[0][j],
                                     i->tex[aux][j],
                                     i->size[0][j],
                                     &i->roi[0][j],
                                     &i->roi[read][j],
                                     &i->roi[aux][j],
                                     fmatrix_x,
                                     cmatrix_len_x,
                                     op_area->left,
                                     fmatrix_y,
                                     cmatrix_len_y,
                                     op_area->top);
          if (cl_err != CL_SUCCESS)
            {
              g_warning("[OpenCL] Error in gegl:gaussian-blur: %s", gegl_cl_errstring(cl_err));
              return FALSE;
            }
        }
    }
  }

  g_free (fmatrix_x);
  g_free (fmatrix_y);

  g_free (cmatrix_x);
  g_free (cmatrix_y);
  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglRectangle rect;
  GeglBuffer *temp;
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle temp_extend;
  gdouble       B, b[4];
  gdouble      *cmatrix;
  gint          cmatrix_len;
  gboolean      force_iir;
  gboolean      force_fir;

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  force_iir = o->filter && !strcmp (o->filter, "iir");
  force_fir = o->filter && !strcmp (o->filter, "fir");

  if (gegl_cl_is_accelerated () && !force_iir)
    if (cl_process(operation, input, output, result))
      return TRUE;

  temp_extend = rect;
  temp_extend.x      = result->x;
  temp_extend.width  = result->width;
  temp = gegl_buffer_new (&temp_extend, babl_format ("RaGaBaA float"));

  if ((force_iir || o->std_dev_x > 1.0) && !force_fir)
    {
      iir_young_find_constants (o->std_dev_x, &B, b);
      iir_young_hor_blur (input, &rect, temp, &temp_extend,  B, b);
    }
  else
    {
      cmatrix_len = fir_gen_convolve_matrix (o->std_dev_x, &cmatrix);
      fir_hor_blur (input, &rect, temp, &temp_extend,
                    cmatrix, cmatrix_len, op_area->left);
      g_free (cmatrix);
    }


  if ((force_iir || o->std_dev_y > 1.0) && !force_fir)
    {
      iir_young_find_constants (o->std_dev_y, &B, b);
      iir_young_ver_blur (temp, &temp_extend, output, result, B, b);
    }
  else
    {
      cmatrix_len = fir_gen_convolve_matrix (o->std_dev_y, &cmatrix);
      fir_ver_blur (temp, &temp_extend, output, result, cmatrix, cmatrix_len,
                    op_area->top);
      g_free (cmatrix);
    }

  g_object_unref (temp);
  return  TRUE;
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
    "name",       "gegl:gaussian-blur",
    "categories", "blur",
    "description",
        _("Performs an averaging of neighboring pixels with the "
          "normal distribution as weighting"),
        NULL);
}

#endif
