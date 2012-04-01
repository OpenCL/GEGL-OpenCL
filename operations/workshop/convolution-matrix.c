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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (a1, _("(1,1) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (1,1)"))
gegl_chant_double (a2, _("(1,2) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (1,2)"))
gegl_chant_double (a3, _("(1,3) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (1,3)"))
gegl_chant_double (a4, _("(1,4) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (1,4)"))
gegl_chant_double (a5, _("(1,5) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (1,5)"))
gegl_chant_double (b1, _("(2,1) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (2,1)"))
gegl_chant_double (b2, _("(2,2) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (2,2)"))
gegl_chant_double (b3, _("(2,3) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (2,3)"))
gegl_chant_double (b4, _("(2,4) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (2,4)"))
gegl_chant_double (b5, _("(2,5) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (2,5)"))
gegl_chant_double (c1, _("(3,1) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (3,1)"))
gegl_chant_double (c2, _("(3,2) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (3,2)"))
gegl_chant_double (c3, _("(3,3) "), -G_MAXINT, G_MAXINT, 1.0,
                   _("Value of the element in position (3,3)"))
gegl_chant_double (c4, _("(3,4) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (3,4)"))
gegl_chant_double (c5, _("(3,5) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (3,5)"))
gegl_chant_double (d1, _("(4,1) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (4,1)"))
gegl_chant_double (d2, _("(4,2) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (4,2)"))
gegl_chant_double (d3, _("(4,3) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (4,3)"))
gegl_chant_double (d4, _("(4,4) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (4,4)"))
gegl_chant_double (d5, _("(4,5) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (4,5)"))
gegl_chant_double (e1, _("(5,1) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (5,1)"))
gegl_chant_double (e2, _("(5,2) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (5,2)"))
gegl_chant_double (e3, _("(5,3) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (5,3)"))
gegl_chant_double (e4, _("(5,4) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (5,4)"))
gegl_chant_double (e5, _("(5,5) "), -G_MAXINT, G_MAXINT, 0.0,
                   _("Value of the element in position (5,5)"))

gegl_chant_double (div, _("Divisor: "), -G_MAXINT, G_MAXINT, 1.0,
                   _("The value of the divisor"))
gegl_chant_double (off, _("Offset: "), -1.0, 1.0, 0.0,
                   _("The value of the offset"))

gegl_chant_boolean (norm, _("Normalize"), TRUE, _("Normalize or not"))

gegl_chant_boolean (red, _("Red"), TRUE, _("Red channel"))
gegl_chant_boolean (green, _("Green"), TRUE, _("Green channel"))
gegl_chant_boolean (blue, _("Blue"), TRUE, _("Blue channel"))
gegl_chant_boolean (alpha, _("Alpha"), TRUE, _("Alpha channel"))

gegl_chant_boolean (weight, _("Alpha-weighting"), TRUE, _("Alpha weighting"))

gegl_chant_string (border, _("Border"), "extend",
                   _("Type of border to choose."
                     "Choices are extend, wrap, crop."
                     "Default is extend"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE        "convolution-matrix.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

#define RESPONSE_RESET 1


#define BIG_MATRIX  /* toggle for 11x11 matrix code experimental*/
#undef BIG_MATRIX


#ifndef BIG_MATRIX
#define MATRIX_SIZE   (5)
#else
#define MATRIX_SIZE   (11)
#endif

#define HALF_WINDOW   (MATRIX_SIZE/2)
#define MATRIX_CELLS  (MATRIX_SIZE*MATRIX_SIZE)
#define DEST_ROWS     (MATRIX_SIZE/2 + 1)
#define CHANNELS      (5)
#define BORDER_MODES  (3)

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left = op_area->right = op_area->top = op_area->bottom = HALF_WINDOW;

  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void
make_matrix (GeglChantO  *o,
             gdouble    **matrix)
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

static void
normalize_o (GeglChantO  *o,
             gdouble    **matrix)
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
      o->off = 0.0;
      o->div = sum;
    }
  else if (sum < 0)
    {
      o->off = 1.0;
      o->div = -sum;
    }
  else
    {
      o->off = 0.5;
      o->div = 1;
    }

}

static void
convolve_pixel(gfloat               *src_buf,
               gfloat               *dst_buf,
               const GeglRectangle  *result,
               const GeglRectangle  *extended,
               const GeglRectangle  *boundary,
               gdouble             **matrix,
               GeglChantO           *o,
               GeglBuffer           *input,
               gint                  xx,
               gint                  yy,
               gdouble               matrixsum)
{
  gint    i, x, y, temp, s_x, s_y;
  gdouble sum;
  gfloat  color[4];
  gint    d_offset, s_offset;
  gint    half;

  gdouble alphasum  = 0.0;

  s_x = 0;
  s_y = 0;

  half = (MATRIX_SIZE / 2) + (MATRIX_SIZE % 2);

  d_offset = ((yy - result->y)*result->width * 4) + (xx - result->x) * 4;
  s_offset = (yy - result->y + HALF_WINDOW) * extended->width * 4 +
    (xx - result->x + HALF_WINDOW) * 4;

  for (i=0; i < 4; i++)
    {
      sum  = 0.0;
      if ((i==0 && o->red) || (i==1 && o->blue) || (i==2 && o->green)
          || (i==3 && o->alpha))
        {
          for (x=0;x < MATRIX_SIZE; x++)
            for (y=0; y < MATRIX_SIZE; y++)
              {
                if (!strcmp(o->border,"wrap"))
                  {
                    s_x = fmod (x+xx, boundary->width);
                    while (s_x < 0)
                      s_x +=boundary->width;

                    s_y = fmod (y+yy, boundary->height);
                    while (s_y < 0)
                      s_y +=boundary->width;
                  }
                else if (!strcmp(o->border,"extend"))
                  {
                    s_x = CLAMP (x+xx, 0, boundary->width);
                    s_y = CLAMP (y+yy, 0, boundary->height);
                  }
                temp = (s_y - extended->y) * extended->width * 4 +
                  (s_x - extended->x) * 4;

                if ((s_x >= extended->x && (s_x < extended->x + extended->width))
                    && (s_y >=extended->y && (s_y < extended->y + extended->height)))
                  {
                    if (i!=3 && o->weight)
                      sum += matrix[x][y] * src_buf[temp + i]
                        * src_buf[temp + 3];
                    else
                      sum += matrix[x][y] * src_buf[temp + i];

                    if (i==3)
                      alphasum += fabs (matrix[x][y] * src_buf[temp+i]);
                  }
                else
                  {
                    gfloat temp_color[4];
                    gegl_buffer_sample (input, s_x, s_y, NULL, temp_color,
                                        babl_format ("RGBA float"),
                                        GEGL_SAMPLER_NEAREST,
                                        GEGL_ABYSS_NONE);
                    if (i!=3 && o->weight)
                      sum += matrix[x][y] * temp_color[i]
                        * temp_color[3];
                    else
                      sum += matrix[x][y] * temp_color[i];

                    if (i==3)
                      alphasum += fabs (matrix[x][y] * temp_color[i]);
                  }
              }
          sum = sum / o->div;

          if (i==3 && o->weight)
            {
              if (alphasum != 0)
                sum = sum * matrixsum / alphasum;
              else sum = 0.0;
            }
          sum += o->off;

          color[i] = sum;
        }
      else
        color[i] = src_buf[s_offset + i];
    }

  for (i=0; i < 4; i++)
    dst_buf[d_offset + i] = color[i];
}



static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy(&result, in_rect);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglRectangle   rect;
  GeglRectangle   boundary = get_effective_area (operation);
  gfloat         *src_buf;
  gfloat         *dst_buf;
  gdouble       **matrix;

  gchar         *type;
  gint           x, y;
  gdouble        matrixsum = 0.0;

  type = "RGBA float";

  matrix = g_new0 (gdouble*, MATRIX_SIZE);

  for (x=0; x < MATRIX_SIZE ;x++)
    matrix[x] = g_new0 (gdouble, MATRIX_SIZE);

  make_matrix (o, matrix);

  if (o->norm)
    normalize_o (o, matrix);

  for (x=0; x < MATRIX_SIZE; x++)
    for (y=0; y < MATRIX_SIZE; y++)
      matrixsum += fabs (matrix[x][y]);

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_new0 (gfloat, rect.width * rect.height * 4);
  dst_buf = g_new0 (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, &rect, 1.0, babl_format (type), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*fill src_buf with wrap pixels if it is the case*/

  if (o->div != 0)
    {
      for (y=result->y; y < result->height + result->y; y++)
        for (x=result->x; x < result->width + result->x; x++)
          convolve_pixel (src_buf, dst_buf, result, &rect, &boundary,
                          matrix, o, input, x, y, matrixsum);

      gegl_buffer_set (output, result, 0, babl_format (type),
                       dst_buf, GEGL_AUTO_ROWSTRIDE);
    }
  else
    gegl_buffer_set (output, &rect, 0, babl_format (type),
                     src_buf, GEGL_AUTO_ROWSTRIDE);



  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
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
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "categories"  , "generic",
    "name"        , "gegl:convolution-matrix",
    "description" ,
    _("Creates image by manually set convolution matrix"),
    NULL);
}

#endif
