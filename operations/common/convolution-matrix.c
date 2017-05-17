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
 * Copyright (C) 1997 Lauri Alanko <la@iki.fi>
 * Copyright 2011 Robert Sasu (sasu.robert@gmail.com)
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (a1, _("(1,1)"), 0.0)
property_double (a2, _("(1,2)"), 0.0)
property_double (a3, _("(1,3)"), 0.0)
property_double (a4, _("(1,4)"), 0.0)
property_double (a5, _("(1,5)"), 0.0)
property_double (b1, _("(2,1)"), 0.0)
property_double (b2, _("(2,2)"), 0.0)
property_double (b3, _("(2,3)"), 0.0)
property_double (b4, _("(2,4)"), 0.0)
property_double (b5, _("(2,5)"), 0.0)
property_double (c1, _("(3,1)"), 0.0)
property_double (c2, _("(3,2)"), 0.0)
property_double (c3, _("(3,3)"), 1.0)
property_double (c4, _("(3,4)"), 0.0)
property_double (c5, _("(3,5)"), 0.0)
property_double (d1, _("(4,1)"), 0.0)
property_double (d2, _("(4,2)"), 0.0)
property_double (d3, _("(4,3)"), 0.0)
property_double (d4, _("(4,4)"), 0.0)
property_double (d5, _("(4,5)"), 0.0)
property_double (e1, _("(5,1)"), 0.0)
property_double (e2, _("(5,2)"), 0.0)
property_double (e3, _("(5,3)"), 0.0)
property_double (e4, _("(5,4)"), 0.0)
property_double (e5, _("(5,5)"), 0.0)

property_double (divisor, _("Divisor"), 1.0)
    ui_range    (-1000.0, 1000.0)
    ui_meta     ("sensitive", "! normalize")

property_double (offset, _("Offset"), 0.0)
    value_range (-1.0, 1.0)
    ui_meta     ("sensitive", "! normalize")

property_boolean (red,   _("Red channel"),   TRUE)
property_boolean (green, _("Green channel"), TRUE)
property_boolean (blue,  _("Blue channel"),  TRUE)
property_boolean (alpha, _("Alpha channel"), TRUE)

property_boolean (normalize,    _("Normalize"),       TRUE)
property_boolean (alpha_weight, _("Alpha-weighting"), TRUE)

property_enum (border, _("Border"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_CLAMP)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     convolution_matrix
#define GEGL_OP_C_SOURCE convolution-matrix.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define MAX_MATRIX_SIZE 5

static gboolean
enough_with_3x3 (GeglProperties *o)
{
  if (o->a1 == 0.0 &&
      o->a2 == 0.0 &&
      o->a3 == 0.0 &&
      o->a4 == 0.0 &&
      o->a5 == 0.0 &&
      o->b1 == 0.0 &&
      o->b5 == 0.0 &&
      o->c1 == 0.0 &&
      o->c5 == 0.0 &&
      o->d1 == 0.0 &&
      o->d5 == 0.0 &&
      o->e1 == 0.0 &&
      o->e2 == 0.0 &&
      o->e3 == 0.0 &&
      o->e4 == 0.0 &&
      o->e5 == 0.0)
  {
    return TRUE;
  }
  return FALSE;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  if (enough_with_3x3 (o))
    op_area->left = op_area->right = op_area->top = op_area->bottom = 1; /* 3 */
  else
    op_area->left = op_area->right = op_area->top = op_area->bottom = 2; /* 5 */

  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void
make_matrix (GeglProperties  *o,
             gfloat           matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
             gint             matrix_size)
{
  if (matrix_size == 3)
  {
    matrix[0][0] = o->b2;
    matrix[0][1] = o->b3;
    matrix[0][2] = o->b4;

    matrix[1][0] = o->c2;
    matrix[1][1] = o->c3;
    matrix[1][2] = o->c4;

    matrix[2][0] = o->d2;
    matrix[2][1] = o->d3;
    matrix[2][2] = o->d4;
  }
  else
  {
    matrix[0][0] = o->a1;
    matrix[0][1] = o->a2;
    matrix[0][2] = o->a3;
    matrix[0][3] = o->a4;
    matrix[0][4] = o->a5;

    matrix[1][0] = o->b1;
    matrix[1][1] = o->b2;
    matrix[1][2] = o->b3;
    matrix[1][3] = o->b4;
    matrix[1][4] = o->b5;

    matrix[2][0] = o->c1;
    matrix[2][1] = o->c2;
    matrix[2][2] = o->c3;
    matrix[2][3] = o->c4;
    matrix[2][4] = o->c5;

    matrix[3][0] = o->d1;
    matrix[3][1] = o->d2;
    matrix[3][2] = o->d3;
    matrix[3][3] = o->d4;
    matrix[3][4] = o->d5;

    matrix[4][0] = o->e1;
    matrix[4][1] = o->e2;
    matrix[4][2] = o->e3;
    matrix[4][3] = o->e4;
    matrix[4][4] = o->e5;
  }
}

static gboolean
normalize_div_off (gfloat  matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
                   gint    matrix_size,
                   gfloat *divisor,
                   gfloat *offset)
{
  gint      x, y;
  gboolean  valid = FALSE;
  gfloat    sum   = 0.0;

  for (y = 0; y < matrix_size; y++)
    for (x = 0; x < matrix_size; x++)
      {
        sum += matrix[x][y];
        if (matrix[x][y] != 0.0)
          valid = TRUE;
      }

  if (sum > 0)
    {
      *offset  = 0.0;
      *divisor = sum;
    }
  else if (sum < 0)
    {
      *offset  = 1.0;
      *divisor = -sum;
    }
  else
    {
      *offset  = 0.5;
      *divisor = 1;
    }

  return valid;
}

static void inline
convolve_pixel_componentwise (GeglProperties       *o,
                gfloat               *src_buf,
                gfloat               *dst_buf,
                const GeglRectangle  *result,
                const GeglRectangle  *extended,
                gfloat                matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
                gint                  matrix_size,
                gint                  d_offset,
                gint                  ss_offset,
                gint                  xx,
                gint                  yy,
                gfloat                matrixsum,
                gfloat                inv_divisor,
                gfloat                offset)
{
  gint   i;
  gint   s_stride = (extended->width - matrix_size) * 4;
  for (i = 0; i < 4; i++)
    {
      gfloat sum = 0.0;
      gint s_offset = ss_offset;

      if ((i == 0 && o->red)   ||
          (i == 1 && o->blue)  ||
          (i == 2 && o->green) ||
          (i == 3 && o->alpha))
        {
          gint x, y;
          for (y = 0; y < matrix_size; y++)
          {
            for (x = 0; x < matrix_size; x++)
              {
                sum += matrix[x][y] * src_buf[s_offset + i];
                s_offset += 4;
              }
            s_offset += s_stride;
          }
          sum = sum * inv_divisor;
          sum += offset;
          dst_buf[d_offset + i] = sum;
        }
      else
        {
          dst_buf[d_offset + i] = src_buf[s_offset + i];
        }
    }
}

static void inline
convolve_pixel (GeglProperties       *o,
                gfloat               *src_buf,
                gfloat               *dst_buf,
                const GeglRectangle  *result,
                const GeglRectangle  *extended,
                gfloat                matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
                gint                  matrix_size,
                gint                  d_offset,
                gint                  ss_offset,
                gint                  xx,
                gint                  yy,
                gfloat                matrixsum,
                gfloat                inv_divisor,
                gfloat                offset)
{
  gint   i;
  gint   s_stride = (extended->width - matrix_size) * 4;
  for (i = 0; i < 4; i++)
    {
      gfloat sum = 0.0;
      gint s_offset = ss_offset;
      gint x, y;
      for (y = 0; y < matrix_size; y++)
      {
        for (x = 0; x < matrix_size; x++)
          {
            sum += matrix[x][y] * src_buf[s_offset + i];
            s_offset += 4;
          }
        s_offset += s_stride;
      }
      sum = sum * inv_divisor;
      sum += offset;
      dst_buf[d_offset + i] = sum;
    }
}

static void inline
convolve_pixel_alpha_weight (GeglProperties       *o,
                             gfloat               *src_buf,
                             gfloat               *dst_buf,
                             const GeglRectangle  *result,
                             const GeglRectangle  *extended,
                             gfloat                matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
                             gint                  matrix_size,
                             gint                  d_offset,
                             gint                  ss_offset,
                             gint                  xx,
                             gint                  yy,
                             gfloat                matrixsum,
                             gfloat                inv_divisor,
                             gfloat                offset)
{
  gint   i;
  gint   s_stride = (extended->width - matrix_size) * 4;

  for (i = 0; i < 3; i++)
    {
      gfloat sum      = 0.0;
      gint   s_offset = ss_offset;
      gint x, y;
      for (y = 0; y < matrix_size; y++)
      {
        for (x = 0; x < matrix_size; x++)
          {
            sum += matrix[x][y] * src_buf[s_offset + i] * src_buf[s_offset + 3];
            s_offset += 4;
          }
        s_offset += s_stride;
      }
      sum = sum * inv_divisor + offset;
      dst_buf[d_offset + i] = sum;
    }
    {
      gfloat sum      = 0.0;
      gfloat alphasum = 0.0;
      gint   s_offset = ss_offset;
      gint x, y;

      for (y = 0; y < matrix_size; y++)
      {
        for (x = 0; x < matrix_size; x++)
          {
            float val = matrix[x][y] * src_buf[s_offset + i];
            sum += val;
            alphasum += fabsf (val);
            s_offset += 4;
          }
        s_offset += s_stride;
      }

      if (alphasum > 0.0)
      {
        sum = sum * inv_divisor;
        sum = sum * matrixsum / alphasum + offset;
      }
      else
        sum = offset;
      dst_buf[d_offset + i] = sum;
    }
}

static void inline
convolve_pixel_alpha_weight_componentwise (GeglProperties       *o,
                             gfloat               *src_buf,
                             gfloat               *dst_buf,
                             const GeglRectangle  *result,
                             const GeglRectangle  *extended,
                             gfloat                matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE],
                             gint                  matrix_size,
                             gint                  d_offset,
                             gint                  ss_offset,
                             gint                  xx,
                             gint                  yy,
                             gfloat                matrixsum,
                             gfloat                inv_divisor,
                             gfloat                offset)
{
  gint   i;
  gint   s_stride = (extended->width - matrix_size) * 4;

  for (i = 0; i < 3; i++)
    {
      gfloat sum      = 0.0;
      gint   s_offset = ss_offset;

      if ((i == 0 && o->red)   ||
          (i == 1 && o->blue)  ||
          (i == 2 && o->green))
        {
          gint x, y;
          for (y = 0; y < matrix_size; y++)
          {
            for (x = 0; x < matrix_size; x++)
              {
                sum += matrix[x][y] * src_buf[s_offset + i] * src_buf[s_offset + 3];
                s_offset += 4;
              }
            s_offset += s_stride;
          }
          sum = sum * inv_divisor + offset;
        }
      else
        {
          sum = src_buf[s_offset + i];
        }
      dst_buf[d_offset + i] = sum;
    }
    {
      gfloat sum      = 0.0;
      gfloat alphasum = 0.0;
      gint   s_offset = ss_offset;

      if (o->alpha)
        {
          gint x, y;

          for (y = 0; y < matrix_size; y++)
          {
            for (x = 0; x < matrix_size; x++)
              {
                float val = matrix[x][y] * src_buf[s_offset + i];
                sum += val;
                alphasum += fabsf (val);
                s_offset += 4;
              }
            s_offset += s_stride;
          }


          if (alphasum != 0)
          {
            sum = sum * inv_divisor;
            sum = sum * matrixsum / alphasum + offset;
          }
          else
            sum = offset;
        }
      else
        {
          sum = src_buf[s_offset + i];
        }
      dst_buf[d_offset + i] = sum;
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  const Babl              *format  = babl_format ("RGBA float");
  GeglRectangle            rect;
  gfloat                  *src_buf;
  gfloat                  *dst_buf;
  gfloat                   matrix[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE]={{0,}};
  gfloat                   matrixsum = 0.0;
  gfloat                   divisor = o->divisor;
  gfloat                   offset = o->offset;
  gfloat                   inv_divisor;
  gint                     x, y;
  gint                     matrix_size = MAX_MATRIX_SIZE;

  if (enough_with_3x3 (o))
    matrix_size = 3;

  make_matrix (o, matrix, matrix_size);

  if (o->normalize)
    normalize_div_off (matrix, matrix_size, &divisor, &offset);
  inv_divisor = 1.0 / divisor;

  for (x = 0; x < matrix_size; x++)
    for (y = 0; y < matrix_size; y++)
      matrixsum += fabsf (matrix[x][y]);

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_new (gfloat, rect.width * rect.height * 4);
  dst_buf = g_new (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, &rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, o->border);

  if (o->divisor != 0)
    {
      gint ss_offset = (result->y - matrix_size/2 - rect.y) * rect.width * 4 +
                       (result->x - matrix_size/2 - rect.x) * 4;
      int d_offset = 0;
      if (o->alpha_weight)
        {
          if (o->red == FALSE || o->green == FALSE || o->blue == FALSE || o->alpha == FALSE)
          {
            gint x, y;
            for (y = result->y; y < result->height + result->y; y++)
            {
              for (x = result->x; x < result->width + result->x; x++)
              {
                convolve_pixel_alpha_weight_componentwise (o, src_buf, dst_buf, result, &rect,
                                           matrix, matrix_size, d_offset, ss_offset, x, y, matrixsum, inv_divisor, offset);
                d_offset += 4;
                ss_offset += 4;
              }
              ss_offset += (rect.width - result->width) * 4;
            }
          }
          else
          {
            gint x, y;
            for (y = result->y; y < result->height + result->y; y++)
            {
              for (x = result->x; x < result->width + result->x; x++)
              {
                convolve_pixel_alpha_weight (o, src_buf, dst_buf, result, &rect,
                                           matrix, matrix_size, d_offset, ss_offset, x, y, matrixsum, inv_divisor, offset);
                d_offset += 4;
                ss_offset += 4;
              }
              ss_offset += (rect.width - result->width) * 4;
            }
          }
        }
      else
        {
          if (o->red == FALSE || o->green == FALSE || o->blue == FALSE || o->alpha == FALSE)
          {
            gint x, y;
            for (y = result->y; y < result->height + result->y; y++)
            {
              for (x = result->x; x < result->width + result->x; x++)
              {
                convolve_pixel_componentwise (o, src_buf, dst_buf, result, &rect,
                                matrix, matrix_size, d_offset, ss_offset, x, y, matrixsum, inv_divisor, offset);
                d_offset += 4;
                ss_offset += 4;
              }
              ss_offset += (rect.width - result->width) * 4;
            }
          }
          else
          {
            gint x, y;
            for (y = result->y; y < result->height + result->y; y++)
            {
              for (x = result->x; x < result->width + result->x; x++)
              {
                convolve_pixel (o, src_buf, dst_buf, result, &rect,
                                matrix, matrix_size, d_offset, ss_offset, x, y, matrixsum, inv_divisor, offset);
                d_offset += 4;
                ss_offset += 4;
              }
              ss_offset += (rect.width - result->width) * 4;
            }
          }
        }
        gegl_buffer_set (output, result, 0, format,
                         dst_buf, GEGL_AUTO_ROWSTRIDE);
    }
  else
    {
      gegl_buffer_set (output, &rect, 0, format,
                       src_buf, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);
  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "categories",  "generic",
    "name",        "gegl:convolution-matrix",
    "title",       _("Convolution Matrix"),
    "description", _("Apply a generic 5x5 convolution matrix"),
    NULL);
}

#endif
