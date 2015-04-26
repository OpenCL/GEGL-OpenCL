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
 *           2014 Daniel Sabo
 *
 * Recursive Gauss IIR Filter as described by Young / van Vliet
 * in "Signal Processing 44 (1995) 139 - 151"
 *
 * NOTE: The IIR filter should not be used for radius < 0.5, since it
 *       becomes very inaccurate.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_gaussian_blur_filter)
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER_AUTO, "auto", N_("Auto"))
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER_FIR,  "fir",  N_("FIR"))
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER_IIR,  "iir",  N_("IIR"))
enum_end (GeglGaussianBlurFilter)

property_double (std_dev_x, _("Size X"), 1.5)
    description (_("Standard deviation for the horizontal axis"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_double (std_dev_y, _("Size Y"), 1.5)
    description (_("Standard deviation for the vertical axis"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_enum  (filter, _("Filter"),
    GeglGaussianBlurFilter, gegl_gaussian_blur_filter,
    GEGL_GAUSSIAN_BLUR_FILTER_AUTO)
    description (_("Optional parameter to override the automatic selection of blur filter"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE gaussian-blur.c

#include "gegl-op.h"
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

  if (GEGL_FLOAT_IS_ZERO (sigma))
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
iir_young_blur_pixels_1D (gfloat  *buf,
                          gint     components,
                          gdouble  B,
                          gdouble *b,
                          gfloat  *w,
                          gint     w_len)
{
  gint wcount, i, c;
  gdouble tmp;
  gdouble recip = 1.0 / b[0];
  gint offset = 0;

  /* forward filter */
  wcount = 0;

  while (wcount < w_len)
    {
      for (c = 0; c < components; ++c)
        {
          tmp = 0;

          for (i = 1; i < 4; i++)
            {
              if (wcount - i >= 0)
                tmp += b[i] * w[(wcount - i) * components + c];
            }

          tmp *= recip;
          tmp += B * buf[offset + c];
          w[wcount * components + c] = tmp;
        }

      wcount++;
      offset += components;
    }

  /* backward filter */
  wcount = w_len - 1;
  offset -= components;

  while (wcount >= 0)
    {
      for (c = 0; c < components; ++c)
        {
          tmp = 0;

          for (i = 1; i < 4; i++)
            {
              if (wcount + i < w_len)
                tmp += b[i] * buf[offset + components * i + c];
            }

          tmp *= recip;
          tmp += B * w[wcount * components + c];
          buf[offset + c] = tmp;
        }

      offset -= components;
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
  const Babl *format = babl_format ("RaGaBaA float");
  const int pixel_count = src_rect->width;
  gfloat *buf     = gegl_malloc (pixel_count * sizeof(gfloat) * 4);
  gfloat *scratch = gegl_malloc (pixel_count * sizeof(gfloat) * 4);
  GeglRectangle read_rect = {src_rect->x, dst_rect->y, src_rect->width, 1};

  for (v = 0; v < dst_rect->height; v++)
    {
      read_rect.y = dst_rect->y + v;
      gegl_buffer_get (src, &read_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_pixels_1D (buf, 4, B, b, scratch, pixel_count);

      gegl_buffer_set (dst, &read_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (buf);
  gegl_free (scratch);
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
  const Babl *format = babl_format ("RaGaBaA float");
  const int pixel_count = src_rect->height;
  gfloat *buf     = gegl_malloc (pixel_count * sizeof(gfloat) * 4);
  gfloat *scratch = gegl_malloc (pixel_count * sizeof(gfloat) * 4);
  GeglRectangle read_rect = {dst_rect->x, src_rect->y, 1, src_rect->height};

  for (u = 0; u < dst_rect->width; u++)
    {
      read_rect.x = dst_rect->x + u;
      gegl_buffer_get (src, &read_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_pixels_1D (buf, 4, B, b, scratch, pixel_count);

      gegl_buffer_set (dst, &read_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (buf);
  gegl_free (scratch);
}

static gint
fir_calc_convolve_matrix_length (gdouble sigma)
{
  return sigma > GEGL_FLOAT_EPSILON ? ceil (sigma) * 6 + 1 : 1;
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

static inline void
fir_get_mean_pixel_1D (gfloat  *src,
                       gfloat  *dst,
                       gint     components,
                       gdouble *cmatrix,
                       gint     matrix_length)
{
  gint    i, c;
  gint    offset;
  gdouble acc[components];

  for (c = 0; c < components; ++c)
    acc[c] = 0;

  offset = 0;

  for (i = 0; i < matrix_length; i++)
    {
      for (c = 0; c < components; ++c)
        acc[c] += src[offset++] * cmatrix[i];
    }

  for (c = 0; c < components; ++c)
    dst[c] = acc[c];
}

static void
fir_hor_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length)
{
  gint        u, v;
  const gint  radius = matrix_length / 2;
  const Babl *format = babl_format ("RaGaBaA float");

  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, dst_rect->width, 1};
  gfloat *dst_buf     = gegl_malloc (write_rect.width * sizeof(gfloat) * 4);

  GeglRectangle read_rect = {dst_rect->x - radius, dst_rect->y, dst_rect->width + 2 * radius, 1};
  gfloat *src_buf    = gegl_malloc (read_rect.width * sizeof(gfloat) * 4);

  for (v = 0; v < dst_rect->height; v++)
    {
      gint offset     = 0;
      read_rect.y     = dst_rect->y + v;
      write_rect.y    = dst_rect->y + v;
      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (u = 0; u < dst_rect->width; u++)
        {
          fir_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 4,
                                 cmatrix,
                                 matrix_length);
          offset += 4;
        }

      gegl_buffer_set (dst, &write_rect, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (src_buf);
  gegl_free (dst_buf);
}

static void
fir_ver_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length)
{
  gint        u,v;
  const gint  radius = matrix_length / 2;
  const Babl *format = babl_format ("RaGaBaA float");

  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, 1, dst_rect->height};
  gfloat *dst_buf    = gegl_malloc (write_rect.height * sizeof(gfloat) * 4);

  GeglRectangle read_rect  = {dst_rect->x, dst_rect->y - radius, 1, dst_rect->height + 2 * radius};
  gfloat *src_buf    = gegl_malloc (read_rect.height * sizeof(gfloat) * 4);

  for (u = 0; u < dst_rect->width; u++)
    {
      gint offset     = 0;
      read_rect.x     = dst_rect->x + u;
      write_rect.x    = dst_rect->x + u;
      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (v = 0; v < dst_rect->height; v++)
        {
          fir_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 4,
                                 cmatrix,
                                 matrix_length);
          offset += 4;
        }

      gegl_buffer_set (dst, &write_rect, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (src_buf);
  gegl_free (dst_buf);
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o    = GEGL_PROPERTIES (operation);

  gfloat fir_radius_x = fir_calc_convolve_matrix_length (o->std_dev_x) / 2;
  gfloat fir_radius_y = fir_calc_convolve_matrix_length (o->std_dev_y) / 2;
  gfloat iir_radius_x = o->std_dev_x * RADIUS_SCALE;
  gfloat iir_radius_y = o->std_dev_y * RADIUS_SCALE;

  /* XXX: these should be calculated exactly considering o->filter, but we just
   * make sure there is enough space */
  area->left = area->right = ceil (MAX (fir_radius_x, iir_radius_x));
  area->top = area->bottom = ceil (MAX (fir_radius_y, iir_radius_y));

  gegl_operation_set_format (operation, "input",
                             babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RaGaBaA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/gaussian-blur.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
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

  cl_mem cl_matrix_x = NULL;
  cl_mem cl_matrix_y = NULL;

  if (!cl_data)
    {
      const char *kernel_name[] = {"fir_ver_blur", "fir_hor_blur", NULL};
      cl_data = gegl_cl_compile_and_build (gaussian_blur_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  cl_matrix_x = gegl_clCreateBuffer (gegl_cl_get_context(),
                                     CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                     matrix_length_x * sizeof(cl_float), dmatrix_x, &cl_err);
  CL_CHECK;

  cl_matrix_y = gegl_clCreateBuffer (gegl_cl_get_context(),
                                     CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                     matrix_length_y * sizeof(cl_float), dmatrix_y, &cl_err);
  CL_CHECK;

  global_ws[0] = aux_rect->width;
  global_ws[1] = aux_rect->height;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[1],
                                    sizeof(cl_mem), (void*)&in_tex,
                                    sizeof(cl_int), (void*)&src_rect->width,
                                    sizeof(cl_mem), (void*)&aux_tex,
                                    sizeof(cl_mem), (void*)&cl_matrix_x,
                                    sizeof(cl_int), (void*)&matrix_length_x,
                                    sizeof(cl_int), (void*)&xoff,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;


  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), (void*)&aux_tex,
                                    sizeof(cl_int), (void*)&aux_rect->width,
                                    sizeof(cl_mem), (void*)&out_tex,
                                    sizeof(cl_mem), (void*)&cl_matrix_y,
                                    sizeof(cl_int), (void*)&matrix_length_y,
                                    sizeof(cl_int), (void*)&yoff,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

  cl_err = gegl_clReleaseMemObject (cl_matrix_x);
  CL_CHECK_ONLY (cl_err);
  cl_err = gegl_clReleaseMemObject (cl_matrix_y);
  CL_CHECK_ONLY (cl_err);

  return FALSE;

error:
  if (cl_matrix_x)
    gegl_clReleaseMemObject (cl_matrix_x);
  if (cl_matrix_y)
    gegl_clReleaseMemObject (cl_matrix_y);

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
  gint err = 0;
  gint j;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o       = GEGL_PROPERTIES (operation);

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
                                              0,
                                              0,
                                              op_area->top,
                                              op_area->bottom);

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_gaussian_blur(i->tex[read],
                             i->tex[0],
                             i->tex[aux],
                             i->size[0],
                             &i->roi[0],
                             &i->roi[read],
                             &i->roi[aux],
                             fmatrix_x,
                             cmatrix_len_x,
                             op_area->left,
                             fmatrix_y,
                             cmatrix_len_y,
                             op_area->top);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }
  }

  g_free (fmatrix_x);
  g_free (fmatrix_y);

  g_free (cmatrix_x);
  g_free (cmatrix_y);
  return !err;
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
  GeglProperties          *o       = GEGL_PROPERTIES (operation);

  GeglRectangle temp_extend;
  gdouble       B, b[4];
  gdouble      *cmatrix;
  gint          cmatrix_len;
  gboolean      horizontal_irr;
  gboolean      vertical_irr;

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  if (o->filter == GEGL_GAUSSIAN_BLUR_FILTER_IIR)
    {
      horizontal_irr = TRUE;
      vertical_irr   = TRUE;
    }
  else if (o->filter == GEGL_GAUSSIAN_BLUR_FILTER_FIR)
    {
      horizontal_irr = FALSE;
      vertical_irr   = FALSE;
    }
  else /* GEGL_GAUSSIAN_BLUR_FILTER_AUTO */
    {
      horizontal_irr = o->std_dev_x > 1.0;
      vertical_irr   = o->std_dev_y > 1.0;
    }

  if (gegl_operation_use_opencl (operation) && !(horizontal_irr | vertical_irr))
    if (cl_process(operation, input, output, result))
      return TRUE;

  gegl_rectangle_intersect (&temp_extend, &rect, gegl_buffer_get_extent (input));
  temp_extend.x      = result->x;
  temp_extend.width  = result->width;
  temp = gegl_buffer_new (&temp_extend, babl_format ("RaGaBaA float"));

  if (horizontal_irr)
    {
      iir_young_find_constants (o->std_dev_x, &B, b);
      iir_young_hor_blur (input, &rect, temp, &temp_extend, B, b);
    }
  else
    {
      cmatrix_len = fir_gen_convolve_matrix (o->std_dev_x, &cmatrix);
      fir_hor_blur (input, temp, &temp_extend, cmatrix, cmatrix_len);
      g_free (cmatrix);
    }

  if (vertical_irr)
    {
      iir_young_find_constants (o->std_dev_y, &B, b);
      iir_young_ver_blur (temp, &rect, output, result, B, b);
    }
  else
    {
      cmatrix_len = fir_gen_convolve_matrix (o->std_dev_y, &cmatrix);
      fir_ver_blur (temp, output, result, cmatrix, cmatrix_len);
      g_free (cmatrix);
    }

  g_object_unref (temp);
  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = TRUE;

  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:gaussian-blur-old",
    "title",       _("Gaussian Blur"),
    "categories",  "blur",
    "description", _("Each result pixel is the average of the neighbouring pixels weighted by a normal distribution with specified standard deviation."),
    NULL);
}

#endif
