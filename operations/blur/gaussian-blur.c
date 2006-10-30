/* This file is an image processing operation for GEGL
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
 *
 * Recursive Gauss IIR Filter as described by Young / van Vliet
 * in "Signal Processing 44 (1995) 139 - 151"
 *
 * NOTE: The IIR filter should not be used for radius < 0.5, since it
 *       becomes very inaccurate.
 */

#if GEGL_CHANT_PROPERTIES

gegl_chant_double (radius_x, 0.0, 500.0, 4.0,
   "Standard deviation for the horizontal axis.")
gegl_chant_double (radius_y, 0.0, 500.0, 4.0,
   "Standard deviation for the vertical axis.")
gegl_chant_string (filter, NULL,
   "Optional parameter to override the automatic selection of blur filter.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            gaussian_blur
#define GEGL_CHANT_DESCRIPTION     "Performs an averaging of neighbouring pixels with the normal distribution as weighting."
#define GEGL_CHANT_SELF            "gaussian-blur.c"
#define GEGL_CHANT_CATEGORIES      "blur"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <math.h>
#include <stdio.h>

static void
iir_young_hor_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b);

static void
iir_young_ver_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b);

static void
iir_young_find_constants (gfloat   radius,
                          gdouble *B,
                          gdouble *b);


static gint
fir_gen_convolve_matrix (gdouble   sigma,
                         gdouble **cmatrix_p);

static void
fir_hor_blur (GeglBuffer *src,
              GeglBuffer *dst,
              gdouble    *cmatrix,
              gint        matrix_length);

static void
fir_ver_blur (GeglBuffer *src,
              GeglBuffer *dst,
              gdouble    *cmatrix,
              gint        matrix_length);

static GeglRect get_source_rect (GeglOperation *operation);

static gboolean
process (GeglOperation *operation)
{
  GeglOperationFilter *filter;
  GeglChantOperation *self;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);

  GeglBuffer *input  = filter->input;
  GeglBuffer *output;

    {
      GeglRect   *result     = gegl_operation_result_rect (operation);
      GeglBuffer *temp_in;
      GeglBuffer *temp = NULL;
      GeglRect    need = get_source_rect (operation);


      if (result->w==0 || result->h==0 || (!self->radius_x && !self->radius_y))
        {
          output = g_object_ref (input);
        }
      else
        {
          temp_in = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", input,
                                 "x",      need.x,
                                 "y",      need.y,
                                 "width",  need.w,
                                 "height", need.h,
                                 NULL);

          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RaGaBaA float"),
                                 "x",      need.x,
                                 "y",      need.y,
                                 "width",  need.w,
                                 "height", need.h,
                                 NULL);

          if (self->radius_x && self->radius_y)
            temp   = g_object_new (GEGL_TYPE_BUFFER,
                                   "format", babl_format ("RaGaBaA float"),
                                   "x",      need.x,
                                   "y",      need.y,
                                   "width",  need.w,
                                   "height", need.h,
                                   NULL);
          else if (!self->radius_x)
            temp = temp_in;

          else if (!self->radius_y)
            temp = output;


          {
            gdouble B, b[4];
            gdouble *cmatrix;
            gint cmatrix_len;
            gchar *filter = self->filter;
            gboolean force_iir = filter && !strcmp (filter, "iir");
            gboolean force_fir = filter && !strcmp (filter, "fir");

            if (self->radius_x)
              {
                if ((force_iir || self->radius_x > 1.0) && !force_fir)
                  {
                    iir_young_find_constants (self->radius_x, &B, b);
                    iir_young_hor_blur (temp_in, temp,   B, b);
                  }
                else
                  {
                    cmatrix_len =
                        fir_gen_convolve_matrix (self->radius_x, &cmatrix);
                    fir_hor_blur (temp_in, temp, cmatrix, cmatrix_len);
                    g_free (cmatrix);
                  }
              }

            if (self->radius_y)
              {
                if ((force_iir || self->radius_y > 1.0) && !force_fir)
                  {
                    iir_young_find_constants (self->radius_y, &B, b);
                    iir_young_ver_blur (temp, output, B, b);
                  }
                else
                  {
                    cmatrix_len =
                        fir_gen_convolve_matrix (self->radius_y, &cmatrix);
                    fir_ver_blur (temp, output, cmatrix, cmatrix_len);
                    g_free (cmatrix);
                  }
              }
          }

          if (temp != output && temp != temp_in)
            g_object_unref (temp);
          g_object_unref (temp_in);
        }

      { 
        GeglBuffer *cropped = g_object_new (GEGL_TYPE_BUFFER,
                                            "source", output,
                                            "x",      result->x,
                                            "y",      result->y,
                                            "width",  result->w,
                                            "height", result->h,
                                            NULL);
        g_object_unref (output);
        filter->output = cropped;
      }
    }

  return  TRUE;
}

static void
iir_young_find_constants (gfloat   sigma,
                          gdouble *B,
                          gdouble *b)
{
  gdouble q;

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

      tmp /= b[0];
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

      tmp /= b[0];
      tmp += B*w[wcount];
      buf[offset] = tmp;

      offset -= delta_offset;
      wcount--;
    }
}

static void
iir_young_hor_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b)
{
  gint v;
  gint c;
  gint w_len;
  gfloat *buf;
  gfloat *w;

  buf = g_malloc0 (src->width * src->height * 4 * 4);
  w   = g_malloc0 (src->width * 4);

  gegl_buffer_get_fmt (src, buf, babl_format ("RaGaBaA float"));

  w_len = src->width;
  for (v=0; v<src->height; v++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             v*src->width*4+c,
                             4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set_fmt (dst, buf, babl_format ("RaGaBaA float"));
  g_free (buf);
  g_free (w);
}

static void
iir_young_ver_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b)
{
  gint v;
  gint c;
  gint w_len;
  gfloat *buf;
  gfloat *w;

  buf = g_malloc0 (src->width * src->height * 4 * 4);
  w   = g_malloc0 (src->height * 4);

  gegl_buffer_get_fmt (src, buf, babl_format ("RaGaBaA float"));

  w_len = src->height;

  for (v=0; v<src->width; v++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             v*4 + c,
                             src->width*4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set_fmt (dst, buf, babl_format ("RaGaBaA float"));
  g_free (buf);
  g_free (w);
}


static gint
fir_gen_convolve_matrix (gdouble   sigma,
                         gdouble **cmatrix_p)
{
  gint     matrix_length;
  gdouble *cmatrix;

  if (!sigma)
    {
      *cmatrix_p = NULL;
      return 0;
    }

  matrix_length = ceil (sigma)*6.0+1.0;
  cmatrix = g_new (gdouble, matrix_length);
  if (!cmatrix)
    return 0;

  {
    gint i,x;
    gdouble y;

    for (i=0; i<matrix_length/2+1; i++)
      {
        x = i - (matrix_length/2);
        y = (1.0/(sigma*sqrt(2*G_PI))) * exp(-(x*x) / (2*sigma*sigma));

        cmatrix[i] = y;
      }

    for (i=matrix_length/2 + 1; i<matrix_length; i++)
      {
        cmatrix[i] = cmatrix[matrix_length-i-1];
      }
  }

  *cmatrix_p = cmatrix;
  return matrix_length;
}


static inline float
fir_get_mean_component (gfloat  * buf,
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
   return buf [offset];
}

static void
fir_hor_blur (GeglBuffer *src,
              GeglBuffer *dst,
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
          dst_buf [offset++] = fir_get_mean_component (src_buf,
                                                       src->width,
                                                       src->height,
                                                       u - (matrix_length/2),
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

void
fir_ver_blur (GeglBuffer *src,
              GeglBuffer *dst,
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
          dst_buf [offset++] = fir_get_mean_component (src_buf,
                                                       src->width,
                                                       src->height,
                                                       u,
                                                       v - (matrix_length/2),
                                                       1,
                                                       matrix_length,
                                                       c,
                                                       cmatrix);
      }


  gegl_buffer_set_fmt (dst, dst_buf, babl_format ("RaGaBaA float"));
  g_free (src_buf);
  g_free (dst_buf);
}




#include <math.h>
static GeglRect
get_defined_region (GeglOperation *operation)
{
  GeglRect  result = {0,0,0,0};
  GeglRect *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                "input");
  GeglChantOperation *blur = GEGL_CHANT_OPERATION (operation);
  gint radius_x       = ceil(blur->radius_x);
  gint radius_y       = ceil(blur->radius_y);
  if (!in_rect)
    return result;

  result = *in_rect;

  if (result.w &&
      result.h)
    {
      result.x-=radius_x*3;
      result.y-=radius_y*3;
      result.w+=radius_x*6;
      result.h+=radius_y*6;
    }

  return result;
}

static GeglRect get_source_rect (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION(operation);
  GeglRect rect;
  gint radius_x, radius_y;

  radius_x = ceil(self->radius_x);
  radius_y = ceil(self->radius_y);

  rect = *gegl_operation_get_requested_region (operation);
  if (rect.w != 0 && 
      rect.h != 0)
    {
      rect.x -= radius_x*3;
      rect.y -= radius_y*3;
      rect.w += radius_x*6;
      rect.h += radius_y*6;
    }

  return rect;
}

static GeglRect
get_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRect       region)
{
  GeglChantOperation *blur   = GEGL_CHANT_OPERATION (self);
  gint                radius_x;
  gint                radius_y;
 
  radius_x = ceil(blur->radius_x);
  radius_y = ceil(blur->radius_y);

  region.x -= radius_x*3;
  region.y -= radius_y*3;
  region.w += radius_x*6;
  region.h += radius_y*6;
  return region;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  GeglRect       need = get_source_rect (self);
  
  gegl_operation_set_source_region (self, "input", &need);

  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_affected_region = get_affected_region;
  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
