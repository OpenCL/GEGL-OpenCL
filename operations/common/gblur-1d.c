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
 * Copyright 2013 Massimo Valentini <mvalentini@src.gnome.org>
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

enum_start (gegl_gblur_1d_policy)
   enum_value (GEGL_GBLUR_1D_ABYSS_NONE,  "none",  N_("None"))
   enum_value (GEGL_GBLUR_1D_ABYSS_CLAMP, "clamp", N_("Clamp"))
   enum_value (GEGL_GBLUR_1D_ABYSS_BLACK, "black", N_("Black"))
   enum_value (GEGL_GBLUR_1D_ABYSS_WHITE, "white", N_("White"))
enum_end (GeglGblur1dPolicy)

enum_start (gegl_gblur_1d_filter)
  enum_value (GEGL_GBLUR_1D_AUTO, "auto", N_("Auto"))
  enum_value (GEGL_GBLUR_1D_FIR,  "fir",  N_("FIR"))
  enum_value (GEGL_GBLUR_1D_IIR,  "iir",  N_("IIR"))
enum_end (GeglGblur1dFilter)

property_double (std_dev, _("Size"), 1.5)
  description (_("Standard deviation (spatial scale factor)"))
  value_range   (0.0, 1500.0)
  ui_range      (0.0, 100.0)
  ui_gamma      (3.0)

property_enum (orientation, _("Orientation"),
               GeglOrientation, gegl_orientation,
               GEGL_ORIENTATION_HORIZONTAL)
  description (_("The orientation of the blur - hor/ver"))

property_enum (filter, _("Filter"),
               GeglGblur1dFilter, gegl_gblur_1d_filter,
               GEGL_GBLUR_1D_AUTO)
  description (_("How the gaussian kernel is discretized"))

property_enum (abyss_policy, _("Abyss policy"), GeglGblur1dPolicy,
               gegl_gblur_1d_policy, GEGL_GBLUR_1D_ABYSS_NONE)
  description (_("How image edges are handled"))

property_boolean (clip_extent, _("Clip to the input extent"), TRUE)
  description (_("Should the output extent be clipped to the input extent"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE gblur-1d.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>


/**********************************************
 *
 * Infinite Impulse Response (IIR)
 *
 **********************************************/

static void
iir_young_find_constants (gfloat   sigma,
                          gdouble *b,
                          gdouble (*m)[3])
{
  const gdouble K1 = 2.44413;
  const gdouble K2 = 1.4281;
  const gdouble K3 = 0.422205;
  const gdouble q = sigma >= 2.5 ?
                    0.98711 * sigma - 0.96330 :
                    3.97156 - 4.14554 * sqrt (1 - 0.26891 * sigma);

  const gdouble b0 = 1.57825 + q*(K1 + q*(    K2 + q *     K3));
  const gdouble b1 =           q*(K1 + q*(2 * K2 + q * 3 * K3));
  const gdouble b2 =         - q*      q*(    K2 + q * 3 * K3);
  const gdouble b3 =           q*      q*          q *     K3;

  const gdouble a1 = b1 / b0;
  const gdouble a2 = b2 / b0;
  const gdouble a3 = b3 / b0;
  const gdouble c  = 1. / ((1+a1-a2+a3) * (1+a2+(a1-a3)*a3));

  m[0][0] = c * (-a3*(a1+a3)-a2 + 1);
  m[0][1] = c * (a3+a1)*(a2+a3*a1);
  m[0][2] = c * a3*(a1+a3*a2);

  m[1][0] = c * (a1+a3*a2);
  m[1][1] = c * (1-a2)*(a2+a3*a1);
  m[1][2] = c * a3*(1-a3*a1-a3*a3-a2);

  m[2][0] = c * (a3*a1+a2+a1*a1-a2*a2);
  m[2][1] = c * (a1*a2+a3*a2*a2-a1*a3*a3-a3*a3*a3-a3*a2+a3);
  m[2][2] = c * a3*(a1+a3*a2);

  b[0] = 1. - (b1 + b2 + b3) / b0;
  b[1] = a1;
  b[2] = a2;
  b[3] = a3;
}

static void
fix_right_boundary (gdouble        *buf,
                    gdouble       (*m)[3],
                    const gdouble   uplus)
{
  gdouble u[3] = { buf[-1] - uplus, buf[-2] - uplus, buf[-3] - uplus };
  gint    i, k;

  for (i = 0; i < 3; i++)
    {
      gdouble tmp = 0.;

      for (k = 0; k < 3; k++)
        tmp += m[i][k] * u[k];

      buf[i] = tmp + uplus;
    }
}

static inline void
iir_young_blur_1D (gfloat        *buf,
                   gdouble       *tmp,
                   const gdouble *b,
                   gdouble      (*m)[3],
                   const gint     len,
                   const gint     nc,
                   GeglAbyssPolicy policy)
{
  gfloat  white[4] = { 1, 1, 1, 1 };
  gfloat  black[4] = { 0, 0, 0, 1 };
  gfloat  none[4]  = { 0, 0, 0, 0 };
  gfloat *uplus, *iminus;
  gint    i, j, c;

  switch (policy)
    {
    case GEGL_ABYSS_CLAMP: default:
      iminus = &buf[nc * 3]; uplus = &buf[nc * (len + 2)]; break;

    case GEGL_ABYSS_NONE:
      iminus = uplus = &none[0]; break;

    case GEGL_ABYSS_WHITE:
      iminus = uplus = &white[0]; break;

    case GEGL_ABYSS_BLACK:
      iminus = uplus = &black[nc == 2 ? 2 : 0]; break;
    }

  for (c = 0; c < nc; ++c)
    {
      for (i = 0; i < 3; ++i)
        tmp[i] = iminus[c];

      for (i = 3; i < 3 + len; ++i)
        {
          tmp[i] = buf[nc * i + c] * b[0];

          for (j = 1; j < 4; ++j)
            tmp[i] += b[j] * tmp[i - j];
        }

      fix_right_boundary (&tmp[3 + len], m, uplus[c]);

      for (i = 3 + len - 1; 3 <= i; --i)
        {
          gdouble tt = 0.;

          for (j = 0; j < 4; ++j)
            tt += b[j] * tmp[i + j];

          buf[nc * i + c] = tmp[i] = tt;
        }
    }
}

static void
iir_young_hor_blur (GeglBuffer          *src,
                    const GeglRectangle *rect,
                    GeglBuffer          *dst,
                    const gdouble       *b,
                    gdouble            (*m)[3],
                    GeglAbyssPolicy      policy,
                    const Babl          *format)
{
  GeglRectangle  cur_row = *rect;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *row = g_new (gfloat, (3 + rect->width + 3) * nc);
  gdouble       *tmp = g_new (gdouble, (3 + rect->width + 3));
  gint           v;

  cur_row.height = 1;

  for (v = 0; v < rect->height; v++)
    {
      cur_row.y = rect->y + v;

      gegl_buffer_get (src, &cur_row, 1.0, format, &row[3 * nc],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_1D (row, tmp, b, m, rect->width, nc, policy);

      gegl_buffer_set (dst, &cur_row, 0, format, &row[3 * nc],
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (tmp);
  g_free (row);
}

static void
iir_young_ver_blur (GeglBuffer          *src,
                    const GeglRectangle *rect,
                    GeglBuffer          *dst,
                    const gdouble       *b,
                    gdouble            (*m)[3],
                    GeglAbyssPolicy      policy,
                    const Babl          *format)
{
  GeglRectangle  cur_col = *rect;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *col = g_new (gfloat, (3 + rect->height + 3) * nc);
  gdouble       *tmp = g_new (gdouble, (3 + rect->height + 3));
  gint           i;

  cur_col.width = 1;

  for (i = 0; i < rect->width; i++)
    {
      cur_col.x = rect->x + i;

      gegl_buffer_get (src, &cur_col, 1.0, format, &col[3 * nc],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_1D (col, tmp, b, m, rect->height, nc, policy);

      gegl_buffer_set (dst, &cur_col, 0, format, &col[3 * nc],
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (tmp);
  g_free (col);
}


/**********************************************
 *
 * Finite Impulse Response (FIR)
 *
 **********************************************/

static inline void
fir_blur_1D (const gfloat *input,
                   gfloat *output,
             const gfloat *cmatrix,
             const gint    clen,
             const gint    len,
             const gint    nc)
{
  gint i;
  for (i = 0; i < len; i++)
    {
      gint c;
      for (c = 0; c < nc; c++)
        {
          gint   index = i * nc + c;
          gfloat acc   = 0.0f;
          gint   m;

          for (m = 0; m < clen; m++)
            acc += input [index + m * nc] * cmatrix [m];

          output [index] = acc;
        }
    }
}

static void
fir_hor_blur (GeglBuffer          *src,
              const GeglRectangle *rect,
              GeglBuffer          *dst,
              gfloat              *cmatrix,
              gint                 clen,
              GeglAbyssPolicy      policy,
              const Babl          *format)
{
  GeglRectangle  cur_row = *rect;
  GeglRectangle  in_row;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *row;
  gfloat        *out;
  gint           v;

  cur_row.height = 1;

  in_row         = cur_row;
  in_row.width  += clen - 1;
  in_row.x      -= clen / 2;

  row = gegl_malloc (sizeof (gfloat) * in_row.width  * nc);
  out = gegl_malloc (sizeof (gfloat) * cur_row.width * nc);

  for (v = 0; v < rect->height; v++)
    {
      cur_row.y = in_row.y = rect->y + v;

      gegl_buffer_get (src, &in_row, 1.0, format, row, GEGL_AUTO_ROWSTRIDE, policy);

      fir_blur_1D (row, out, cmatrix, clen, rect->width, nc);

      gegl_buffer_set (dst, &cur_row, 0, format, out, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (out);
  gegl_free (row);
}

static void
fir_ver_blur (GeglBuffer          *src,
              const GeglRectangle *rect,
              GeglBuffer          *dst,
              gfloat              *cmatrix,
              gint                 clen,
              GeglAbyssPolicy      policy,
              const Babl          *format)
{
  GeglRectangle  cur_col = *rect;
  GeglRectangle  in_col;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *col;
  gfloat        *out;
  gint           v;

  cur_col.width = 1;

  in_col         = cur_col;
  in_col.height += clen - 1;
  in_col.y      -= clen / 2;

  col = gegl_malloc (sizeof (gfloat) * in_col.height  * nc);
  out = gegl_malloc (sizeof (gfloat) * cur_col.height * nc);

  for (v = 0; v < rect->width; v++)
    {
      cur_col.x = in_col.x = rect->x + v;

      gegl_buffer_get (src, &in_col, 1.0, format, col, GEGL_AUTO_ROWSTRIDE, policy);

      fir_blur_1D (col, out, cmatrix, clen, rect->height, nc);

      gegl_buffer_set (dst, &cur_col, 0, format, out, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (out);
  gegl_free (col);
}


#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/gblur-1d.cl.h"

static GeglClRunData *cl_data = NULL;


static gboolean
cl_gaussian_blur (cl_mem                 in_tex,
                  cl_mem                 out_tex,
                  const GeglRectangle   *roi,
                  cl_mem                 cl_cmatrix,
                  gint                   clen,
                  GeglOrientation        orientation)
{
  cl_int cl_err = 0;
  size_t global_ws[2];
  gint   kernel_num;

  if (!cl_data)
    {
      const char *kernel_name[] = {"fir_ver_blur", "fir_hor_blur", NULL};
      cl_data = gegl_cl_compile_and_build (gblur_1d_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  if (orientation == GEGL_ORIENTATION_VERTICAL)
    kernel_num = 0;
  else
    kernel_num = 1;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[kernel_num],
                                    sizeof(cl_mem), (void*)&in_tex,
                                    sizeof(cl_mem), (void*)&out_tex,
                                    sizeof(cl_mem), (void*)&cl_cmatrix,
                                    sizeof(cl_int), (void*)&clen,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[kernel_num], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
fir_cl_process (GeglBuffer            *input,
                GeglBuffer            *output,
                const GeglRectangle   *result,
                const Babl            *format,
                gfloat                *cmatrix,
                gint                   clen,
                GeglOrientation        orientation,
                GeglAbyssPolicy        abyss)
{
  gboolean              err = FALSE;
  cl_int                cl_err;
  cl_mem                cl_cmatrix = NULL;
  GeglBufferClIterator *i;
  gint                  read;
  gint                  left, right, top, bottom;

  if (orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      right = left = clen / 2;
      top = bottom = 0;
    }
  else
    {
      right = left = 0;
      top = bottom = clen / 2;
    }

  i = gegl_buffer_cl_iterator_new (output,
                                   result,
                                   format,
                                   GEGL_CL_BUFFER_WRITE);

  read = gegl_buffer_cl_iterator_add_2 (i,
                                        input,
                                        result,
                                        format,
                                        GEGL_CL_BUFFER_READ,
                                        left, right,
                                        top, bottom,
                                        abyss);

  cl_cmatrix = gegl_clCreateBuffer (gegl_cl_get_context(),
                                    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                    clen * sizeof(cl_float), cmatrix, &cl_err);
  CL_CHECK;

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_gaussian_blur(i->tex[read],
                             i->tex[0],
                             &i->roi[0],
                             cl_cmatrix,
                             clen,
                             orientation);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  cl_err = gegl_clReleaseMemObject (cl_cmatrix);
  CL_CHECK;

  cl_cmatrix = NULL;

  return !err;

error:
  if (cl_cmatrix)
    gegl_clReleaseMemObject (cl_cmatrix);

  return FALSE;
}

static gfloat
gaussian_func_1d (gfloat x,
                  gfloat sigma)
{
  return (1.0 / (sigma * sqrt (2.0 * G_PI))) *
          exp (-(x * x) / (2.0 * sigma * sigma));
}

static gint
fir_calc_convolve_matrix_length (gfloat sigma)
{
#if 1
  /* an arbitrary precision */
  gint clen = sigma > GEGL_FLOAT_EPSILON ? ceil (sigma * 6.5) : 1;
  clen = clen + ((clen + 1) % 2);
  return clen;
#else
  if (sigma > GEGL_FLOAT_EPSILON)
    {
      gint x = 0;
      while (gaussian_func_1d (x, sigma) > 1e-3)
        x++;
      return 2 * x + 1;
    }
  else return 1;
#endif
}

static gint
fir_gen_convolve_matrix (gfloat   sigma,
                         gfloat **cmatrix)
{
  gint    clen;
  gfloat *cmatrix_p;

  clen = fir_calc_convolve_matrix_length (sigma);

  *cmatrix  = gegl_malloc (sizeof (gfloat) * clen);
  cmatrix_p = *cmatrix;

  if (clen == 1)
    {
      cmatrix_p [0] = 1;
    }
  else
    {
      gint    i;
      gdouble sum = 0;
      gint    half_clen = clen / 2;

      for (i = 0; i < clen; i++)
        {
          cmatrix_p [i] = gaussian_func_1d (i - half_clen, sigma);
          sum += cmatrix_p [i];
        }

      for (i = 0; i < clen; i++)
        {
          cmatrix_p [i] /= sum;
        }
    }

  return clen;
}

static GeglGblur1dFilter
filter_disambiguation (GeglGblur1dFilter filter,
                       gfloat            std_dev)
{
  if (filter == GEGL_GBLUR_1D_AUTO)
    {
      /* Threshold 1.0 is arbitrary */
      if (std_dev < 1.0)
        filter = GEGL_GBLUR_1D_FIR;
      else
        filter = GEGL_GBLUR_1D_IIR;
    }
  return filter;
}


/**********************************************
 *
 * GEGL Operation API
 *
 **********************************************/

static void
gegl_gblur_1d_prepare (GeglOperation *operation)
{
  const Babl *src_format = gegl_operation_get_source_format (operation, "input");
  const char *format     = "RaGaBaA float";

  /*
   * FIXME: when the abyss policy is _NONE, the behavior at the edge
   *        depends on input format (with or without an alpha component)
   */
  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (model == babl_model ("RGB") || model == babl_model ("R'G'B'"))
        format = "RGB float";
      else if (model == babl_model ("Y") || model == babl_model ("Y'"))
        format = "Y float";
      else if (model == babl_model ("YA") || model == babl_model ("Y'A") ||
               model == babl_model ("YaA") || model == babl_model ("Y'aA"))
        format = "YaA float";
    }

  gegl_operation_set_format (operation, "output", babl_format (format));
}

static GeglRectangle
gegl_gblur_1d_enlarge_extent (GeglProperties          *o,
                              const GeglRectangle *input_extent)
{
  gint clen = fir_calc_convolve_matrix_length (o->std_dev);

  GeglRectangle bounding_box = *input_extent;

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      bounding_box.x     -= clen / 2;
      bounding_box.width += clen - 1;
    }
  else
    {
      bounding_box.y      -= clen / 2;
      bounding_box.height += clen - 1;
    }

  return bounding_box;
}

static GeglRectangle
gegl_gblur_1d_get_required_for_output (GeglOperation       *operation,
                                       const gchar         *input_pad,
                                       const GeglRectangle *output_roi)
{
  GeglRectangle        required_for_output = { 0, };
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  GeglGblur1dFilter    filter  = filter_disambiguation (o->filter, o->std_dev);

  if (filter == GEGL_GBLUR_1D_IIR)
    {
      const GeglRectangle *in_rect =
        gegl_operation_source_get_bounding_box (operation, input_pad);

      if (in_rect)
        {
          if (!gegl_rectangle_is_infinite_plane (in_rect))
            {
              required_for_output = *output_roi;

              if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
                {
                  required_for_output.x     = in_rect->x;
                  required_for_output.width = in_rect->width;
                }
              else
                {
                  required_for_output.y      = in_rect->y;
                  required_for_output.height = in_rect->height;
                }

              if (!o->clip_extent)
                required_for_output =
                  gegl_gblur_1d_enlarge_extent (o, &required_for_output);
            }
          /* pass-through case */
          else
            return *output_roi;
        }
    }
  else
    {
      required_for_output = gegl_gblur_1d_enlarge_extent (o, output_roi);
    }

  return required_for_output;
}

static GeglRectangle
gegl_gblur_1d_get_bounding_box (GeglOperation *operation)
{
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  if (! in_rect)
    return *GEGL_RECTANGLE (0, 0, 0, 0);

  if (gegl_rectangle_is_infinite_plane (in_rect))
    return *in_rect;

  if (o->clip_extent)
    {
      return *in_rect;
    }
  else
    {
      /* We use the FIR convolution length for both the FIR and the IIR case */
      return gegl_gblur_1d_enlarge_extent (o, in_rect);
    }
}

static GeglRectangle
gegl_gblur_1d_get_cached_region (GeglOperation       *operation,
                                 const GeglRectangle *output_roi)
{
  GeglRectangle      cached_region;
  GeglProperties        *o       = GEGL_PROPERTIES (operation);
  GeglGblur1dFilter  filter  = filter_disambiguation (o->filter, o->std_dev);

  cached_region = *output_roi;

  if (filter == GEGL_GBLUR_1D_IIR)
    {
      const GeglRectangle in_rect =
        gegl_gblur_1d_get_bounding_box (operation);

      if (! gegl_rectangle_is_empty (&in_rect) &&
          ! gegl_rectangle_is_infinite_plane (&in_rect))
        {
          GeglProperties *o = GEGL_PROPERTIES (operation);

          cached_region = *output_roi;

          if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
            {
              cached_region.x     = in_rect.x;
              cached_region.width = in_rect.width;
            }
          else
            {
              cached_region.y      = in_rect.y;
              cached_region.height = in_rect.height;
            }
        }
    }

  return cached_region;
}

static GeglAbyssPolicy
to_gegl_policy (GeglGblur1dPolicy policy)
{
  switch (policy)
    {
    case (GEGL_GBLUR_1D_ABYSS_NONE):
      return GEGL_ABYSS_NONE;
      break;
    case (GEGL_GBLUR_1D_ABYSS_CLAMP):
      return GEGL_ABYSS_CLAMP;
      break;
    case (GEGL_GBLUR_1D_ABYSS_WHITE):
      return GEGL_ABYSS_WHITE;
      break;
    case (GEGL_GBLUR_1D_ABYSS_BLACK):
      return GEGL_ABYSS_BLACK;
      break;
    default:
      g_warning ("gblur-1d: unsupported abyss policy");
      return GEGL_ABYSS_NONE;
    }
}

static gboolean
gegl_gblur_1d_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       GeglBuffer          *output,
                       const GeglRectangle *result,
                       gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "output");

  GeglGblur1dFilter filter;
  GeglAbyssPolicy   abyss_policy = to_gegl_policy (o->abyss_policy);

  filter = filter_disambiguation (o->filter, o->std_dev);

  if (filter == GEGL_GBLUR_1D_IIR)
    {
      gdouble b[4], m[3][3];

      iir_young_find_constants (o->std_dev, b, m);

      if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
        iir_young_hor_blur (input, result, output, b, m, abyss_policy, format);
      else
        iir_young_ver_blur (input, result, output, b, m, abyss_policy, format);
    }
  else
    {
      gfloat *cmatrix;
      gint    clen;

      clen = fir_gen_convolve_matrix (o->std_dev, &cmatrix);

      /* FIXME: implement others format cases */
      if (gegl_operation_use_opencl (operation) &&
          format == babl_format ("RaGaBaA float"))
        if (fir_cl_process(input, output, result, format,
                           cmatrix, clen, o->orientation, abyss_policy))
          return TRUE;

      if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
        fir_hor_blur (input, result, output, cmatrix, clen, abyss_policy, format);
      else
        fir_ver_blur (input, result, output, cmatrix, clen, abyss_policy, format);

      gegl_free (cmatrix);
    }

  return  TRUE;
}

/* Pass-through when trying to perform IIR on an infinite plane
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;
  GeglProperties          *o = GEGL_PROPERTIES (operation);
  GeglGblur1dFilter    filter  = filter_disambiguation (o->filter, o->std_dev);

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (filter == GEGL_GBLUR_1D_IIR)
    {
      const GeglRectangle *in_rect =
        gegl_operation_source_get_bounding_box (operation, "input");

      if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
        {
          gpointer in = gegl_operation_context_get_object (context, "input");
          gegl_operation_context_take_object (context, "output",
                                              g_object_ref (G_OBJECT (in)));
          return TRUE;
        }
    }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = gegl_gblur_1d_process;
  operation_class->prepare                 = gegl_gblur_1d_prepare;
  operation_class->process                 = operation_process;
  operation_class->get_bounding_box        = gegl_gblur_1d_get_bounding_box;
  operation_class->get_required_for_output = gegl_gblur_1d_get_required_for_output;
  operation_class->get_cached_region       = gegl_gblur_1d_get_cached_region;
  operation_class->opencl_support          = TRUE;
  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:gblur-1d",
    "categories", "hidden:blur",
    "description",
        _("Performs an averaging of neighboring pixels with the "
          "normal distribution as weighting"),
        NULL);
}

#endif
