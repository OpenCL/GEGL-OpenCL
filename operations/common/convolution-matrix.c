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

property_double (offset, _("Offset"), 0.0)
    value_range (-1.0, 1.0)

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
#define GEGL_OP_C_SOURCE convolution-matrix.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define MATRIX_SIZE   (5)
#define HALF_WINDOW   (MATRIX_SIZE/2)
#define CHANNELS      (5)

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left = op_area->right = op_area->top = op_area->bottom = HALF_WINDOW;

  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void
make_matrix (GeglProperties  *o,
             gdouble        **matrix)
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

static gboolean
normalize_o (GeglProperties  *o,
             gdouble        **matrix)
{
  gint      x, y;
  gboolean  valid = FALSE;
  gfloat    sum   = 0.0;

  for (y = 0; y < MATRIX_SIZE; y++)
    for (x = 0; x < MATRIX_SIZE; x++)
      {
        sum += matrix[x][y];
        if (matrix[x][y] != 0.0)
          valid = TRUE;
      }

  if (sum > 0)
    {
      o->offset  = 0.0;
      o->divisor = sum;
    }
  else if (sum < 0)
    {
      o->offset  = 1.0;
      o->divisor = -sum;
    }
  else
    {
      o->offset  = 0.5;
      o->divisor = 1;
    }

  return valid;
}

static void
convolve_pixel (GeglProperties       *o,
                gfloat               *src_buf,
                gfloat               *dst_buf,
                const GeglRectangle  *result,
                const GeglRectangle  *extended,
                gdouble             **matrix,
                gint                  xx,
                gint                  yy,
                gdouble               matrixsum)
{
  gfloat  color[4];
  gint    d_offset;
  gint    s_offset;
  gint    i;

  d_offset = (yy - result->y) * result->width * 4 +
             (xx - result->x) * 4;

  for (i = 0; i < 4; i++)
    {
      gdouble sum      = 0.0;
      gdouble alphasum = 0.0;

      if ((i == 0 && o->red)   ||
          (i == 1 && o->blue)  ||
          (i == 2 && o->green) ||
          (i == 3 && o->alpha))
        {
          gint x, y;

          for (x = 0; x < MATRIX_SIZE; x++)
            for (y = 0; y < MATRIX_SIZE; y++)
              {
                gint s_x = x + xx - HALF_WINDOW;
                gint s_y = y + yy - HALF_WINDOW;

                s_offset = (s_y - extended->y) * extended->width * 4 +
                           (s_x - extended->x) * 4;

                if (i != 3 && o->alpha_weight)
                  sum += matrix[x][y] * src_buf[s_offset + i] * src_buf[s_offset + 3];
                else
                  sum += matrix[x][y] * src_buf[s_offset + i];

                if (i == 3)
                  alphasum += fabs (matrix[x][y] * src_buf[s_offset + i]);
              }

          sum = sum / o->divisor;

          if (i == 3 && o->alpha_weight)
            {
              if (alphasum != 0)
                sum = sum * matrixsum / alphasum;
              else
                sum = 0.0;
            }

          sum += o->offset;

          color[i] = sum;
        }
      else
        {
          s_offset = (yy - result->y + HALF_WINDOW) * extended->width * 4 +
                     (xx - result->x + HALF_WINDOW) * 4;

          color[i] = src_buf[s_offset + i];
        }
    }

  for (i = 0; i < 4; i++)
    dst_buf[d_offset + i] = color[i];
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
  gdouble                **matrix;
  gdouble                  matrixsum = 0.0;
  gint                     x, y;

  matrix = g_new0 (gdouble *, MATRIX_SIZE);

  for (x = 0; x < MATRIX_SIZE ;x++)
    matrix[x] = g_new0 (gdouble, MATRIX_SIZE);

  make_matrix (o, matrix);

  if (o->normalize)
    normalize_o (o, matrix);

  for (x = 0; x < MATRIX_SIZE; x++)
    for (y = 0; y < MATRIX_SIZE; y++)
      matrixsum += fabs (matrix[x][y]);

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_new0 (gfloat, rect.width * rect.height * 4);
  dst_buf = g_new0 (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, &rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, o->border);

  if (o->divisor != 0)
    {
      for (y = result->y; y < result->height + result->y; y++)
        for (x = result->x; x < result->width + result->x; x++)
          convolve_pixel (o, src_buf, dst_buf, result, &rect,
                          matrix, x, y, matrixsum);

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
    "description", _("Apply a generic 5x5 convolution matrix"),
    NULL);
}

#endif
