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
 * Copyright 2016 Miroslav Talasek <miroslav.talasek@seznam.cz>
 *
 * Wavelet blur used in wavelet decompose filter
 *  theory is from original wavelet plugin
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


property_double (radius, _("Radius"), 1.0)
    description (_("Radius of the wavelet blur"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 256.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("radius", "blur")


#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     wavelet_blur
#define GEGL_OP_C_SOURCE wavelet-blur.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>


static gint
wav_gen_convolve_matrix (gdouble   radius,
                         gdouble **cmatrix_p);



static gint
wav_calc_convolve_matrix_length (gdouble radius)
{
  return ceil (radius) * 2 + 1;
}

static gint
wav_gen_convolve_matrix (gdouble   radius,
                         gdouble **cmatrix_p)
{
  gint     matrix_length;
  gdouble *cmatrix;

  matrix_length = wav_calc_convolve_matrix_length (radius);
  cmatrix = g_new (gdouble, matrix_length);
  if (!cmatrix)
    return 0;

  if (matrix_length == 1)
    {
      cmatrix[0] = 1;
    }
  else
    {
      gint i;

      for (i = 0; i < matrix_length; i++)
        {
          if (i == 0 || i == matrix_length - 1)
            {
              cmatrix[i] = 0.25;
            }
          else if (i == matrix_length / 2)
            {
              cmatrix[i] = 0.5;
            }
          else
            {
              cmatrix[i] = 0;
            }
        }
    }

  *cmatrix_p = cmatrix;
  return matrix_length;
}

static inline void
wav_get_mean_pixel_1D (gfloat  *src,
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
wav_hor_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length,
              const Babl          *format)
{
  gint        u, v;
  const gint  radius = matrix_length / 2;
  const gint  nc = babl_format_get_n_components (format);
  
  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, dst_rect->width, 1};
  gfloat *dst_buf     = gegl_malloc (write_rect.width * sizeof(gfloat) * nc);

  GeglRectangle read_rect = {dst_rect->x - radius, dst_rect->y, dst_rect->width + matrix_length -1, 1};
  gfloat *src_buf    = gegl_malloc (read_rect.width * sizeof(gfloat) * nc);

  for (v = 0; v < dst_rect->height; v++)
    {
      gint offset     = 0;
      read_rect.y     = dst_rect->y + v;
      write_rect.y    = dst_rect->y + v;
      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (u = 0; u < dst_rect->width; u++)
        {
          wav_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 nc,
                                 cmatrix,
                                 matrix_length);
          offset += nc;
        }

      gegl_buffer_set (dst, &write_rect, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (src_buf);
  gegl_free (dst_buf);
}

static void
wav_ver_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gdouble             *cmatrix,
              gint                 matrix_length,
              const Babl          *format)
{
  gint        u,v;
  const gint  radius = matrix_length / 2;
  const gint  nc = babl_format_get_n_components (format);

  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, 1, dst_rect->height};
  gfloat *dst_buf    = gegl_malloc (write_rect.height * sizeof(gfloat) * nc);

  GeglRectangle read_rect  = {dst_rect->x, dst_rect->y - radius , 1, dst_rect->height + matrix_length -1};
  gfloat *src_buf    = gegl_malloc (read_rect.height * sizeof(gfloat) * nc);

  for (u = 0; u < dst_rect->width; u++)
    {
      gint offset     = 0;
      read_rect.x     = dst_rect->x + u;
      write_rect.x    = dst_rect->x + u;
      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
 
      for (v = 0; v < dst_rect->height; v++)
        {
          wav_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 nc,
                                 cmatrix,
                                 matrix_length);
          offset += nc;
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


  /* XXX: these should be calculated exactly considering o->filter, but we just
   * make sure there is enough space */
  area->left = area->right = ceil (o->radius);
  area->top = area->bottom = ceil (o->radius);

  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B' float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B' float"));
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
  const Babl *format = gegl_operation_get_format (operation, "output");

  GeglRectangle temp_extend;
  gdouble      *cmatrix;
  gint          cmatrix_len;

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;


  gegl_rectangle_intersect (&temp_extend, &rect, gegl_buffer_get_extent (input));
  temp_extend.x      = result->x;
  temp_extend.width  = result->width;
  temp = gegl_buffer_new (&temp_extend, format);

  cmatrix_len = wav_gen_convolve_matrix (o->radius, &cmatrix);
  wav_hor_blur (input, temp, &temp_extend, cmatrix, cmatrix_len, format);
  wav_ver_blur (temp, output, result, cmatrix, cmatrix_len, format);
  g_free (cmatrix);

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
  operation_class->opencl_support = FALSE;

  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:wavelet-blur",
    "title",       _("Wavelet Blur"),
    "categories",  "blur",
    "description", _("This blur is used for the wavelet decomposition filter, each pixel is computed from another by the HAT transform"),
    NULL);
}

#endif
