/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Dominik Ernst <dernst@gmx.de>
 */
#ifdef CHANT_SELF
 
chant_double (radius, 0.0, 200.0, 4.0, "radius of square pixel region.")

#else

#define CHANT_FILTER
#define CHANT_NAME            gaussian_blur
#define CHANT_DESCRIPTION     "Performs an averaging of neighbouring pixels with the normal distribution as weighting."
#define CHANT_SELF            "gaussian-blur.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"

#include <math.h>

static void
hor_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius,
          gdouble    *cmatrix,
          gint        matrix_length);

static void
ver_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius,
          gdouble    *cmatrix,
          gint       matrix_length);

static gint
gen_convolve_matrix (gdouble   radius,
                     gdouble **cmatrix_p);



/* Actual image processing code
 ************************************************************************/
static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationFilter *filter;
  ChantInstance *self;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = CHANT_INSTANCE (operation);

  GeglBuffer *input  = filter->input;
  GeglBuffer *output;

  if(strcmp("output", output_prop))
    return FALSE;

    {
      GeglRect   *result = gegl_operation_result_rect (operation);
      GeglBuffer *temp_in;
      GeglBuffer *temp;
      gdouble    *cmatrix = NULL;
      gint        matrix_length;

      if (result->w==0 ||
          result->h==0)
        {
          output = g_object_ref (input);
        }
      else
        {
          temp_in = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", input,
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->w,
                                 "height", result->h,
                                 NULL);
          temp   = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", gegl_buffer_get_format (input),
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->w,
                                 "height", result->h,
                                 NULL);

          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", gegl_buffer_get_format (input),
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->w,
                                 "height", result->h,
                                 NULL);

          matrix_length = gen_convolve_matrix(self->radius, &cmatrix);

          hor_blur (temp_in, temp,  self->radius, cmatrix, matrix_length);
          ver_blur (temp, output, self->radius, cmatrix, matrix_length);
          
          g_object_unref (temp);
          g_object_unref (temp_in);

          if (cmatrix)
            g_free(cmatrix);
        }

      if (filter->output)
        g_object_unref (filter->output);
      filter->output = output;
    }
  return  TRUE;
}

static inline float
get_mean_component (gfloat  * buf,
                    gint      buf_width,
                    gint      buf_height,
                    gint      x0,
                    gint      y0,
                    gint      width,
                    gint      height,
                    gint      component,
                    gdouble * cmatrix)
{
  gint    x, y;
  gdouble acc=0, count=0.0;
  gint    mcount=0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            acc += buf [offset] * cmatrix[mcount];
            count += cmatrix[mcount];
          }
        offset+=4;
        mcount++;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
   if (count)
     return acc/count;
   /* return 0.0; */
   return buf [offset];
}

static void
hor_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius,
          gdouble    *cmatrix,
          gint        matrix_length)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (src->width * src->height * 4 * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 4);

  gegl_buffer_get_fmt (src, src_buf, babl_format ("RaGaBaA float"));

  offset = 0;
  for (v=0; v<dst->height; v++)
    for (u=0; u<dst->width; u++)
      {
        gint i;

        for (i=0; i<4; i++)
          dst_buf [offset++] = get_mean_component (src_buf,
                               src->width,
                               src->height,
                               u - (matrix_length/2-1),
                               v,
                               matrix_length,
                               1,
                               i,
                               cmatrix);
      }

  gegl_buffer_set_fmt (dst, dst_buf, babl_format ("RaGaBaA float"));
  g_free (src_buf);
  g_free (dst_buf);
}


static void
ver_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius,
          gdouble    *cmatrix,
          gint        matrix_length)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (src->width * src->height * 4 * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 4);
  
  gegl_buffer_get_fmt (src, src_buf, babl_format ("RaGaBaA float"));

  offset=0;
  for (v=0; v<dst->height; v++)
    for (u=0; u<dst->width; u++)
      {
        gint c;

        for (c=0; c<4; c++)
          dst_buf [offset++] =
           get_mean_component (src_buf,
                               src->width,
                               src->height,
                               u,
                               v - (matrix_length/2-1),
                               1,
                               matrix_length,
                               c,
                               cmatrix);
      }

  gegl_buffer_set_fmt (dst, dst_buf, babl_format ("RaGaBaA float"));
  g_free (src_buf);
  g_free (dst_buf);
}

static gboolean
calc_have_rect (GeglOperation *operation)
{
  GeglRect *in_rect = gegl_operation_get_have_rect (operation, "input");
  ChantInstance *blur = CHANT_INSTANCE (operation);
  gint       radius = ceil(blur->radius);
  if (!in_rect)
    return FALSE;

  gegl_operation_set_have_rect (operation, 
     in_rect->x-radius, in_rect->y-radius,
     in_rect->w+radius*2, in_rect->h+radius*2);
  return TRUE;
}


static gboolean
calc_need_rect (GeglOperation *self)
{
  ChantInstance *blur = CHANT_INSTANCE (self);
  GeglRect  *need   = gegl_operation_need_rect (self);
  gint       radius = ceil(blur->radius);

  gegl_operation_set_need_rect (self, "input",
     need->x - radius, need->y - radius,
     need->w + radius, need->h + radius);
  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->calc_have_rect = calc_have_rect;
  operation_class->calc_need_rect = calc_need_rect;
}

static gint
gen_convolve_matrix (gdouble   radius,
                     gdouble **cmatrix_p)
{
  gdouble *cmatrix;
  gdouble  sigma;
  gint     matrix_length;

  sigma = (abs (radius) / 3.0)+1;
  matrix_length = 6*sigma + 1;

  cmatrix = g_new (gdouble, matrix_length);
  if (!cmatrix)
    return 0;

  {
    gint i, x;
    gdouble y;

    for (i=0; i<matrix_length/2+1; i++)
      {
        x = i - (matrix_length/2+1);
        y = (1.0/(sigma*sqrt(2*M_PI))) * exp(-(x*x) / (2*sigma*sigma));

        cmatrix[i] = y;
      }

    for (i=matrix_length/2 + 1; i<matrix_length; i++)
      cmatrix[i] = cmatrix[matrix_length-i-1];
  }

  *cmatrix_p = cmatrix;
  return matrix_length;
}

#endif
