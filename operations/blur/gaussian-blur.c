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

#if GEGL_CHANT_PROPERTIES

gegl_chant_double (std_dev_x, 0.0, 200.0, 4.0,
   "Standard deviation for the horizontal axis. (multiply by ~2 to get radius)")
gegl_chant_double (std_dev_y, 0.0, 200.0, 4.0,
   "Standard deviation for the vertical axis. (multiply by ~2 to get radius.)")
gegl_chant_string (filter, NULL,
   "Optional parameter to override the automatic selection of blur filter.")

#else

#define GEGL_CHANT_NAME            gaussian_blur
#define GEGL_CHANT_SELF            "gaussian-blur.c"
#define GEGL_CHANT_DESCRIPTION     "Performs an averaging of neighbouring pixels with the normal distribution as weighting."
#define GEGL_CHANT_CATEGORIES      "blur"

#define GEGL_CHANT_AREA_FILTER
#include "gegl-chant.h"

#include <math.h>
#include <stdio.h>

#define RADIUS_SCALE   4

static void
iir_young_hor_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b);

static void
iir_young_ver_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b,
                    gint        offset);

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
              gint        matrix_length,
              gint        offsetX,
              gint        offsetY);

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *temp;
  GeglBuffer          *output;

  self = GEGL_CHANT_OPERATION (operation);
  input = gegl_operation_get_source (operation, context_id, "input");
  output = gegl_operation_get_target (operation, context_id, "output");
  temp  = gegl_buffer_new (gegl_buffer_get_extent (input),
                           babl_format ("RaGaBaA float"));

  {
    gdouble B, b[4];
    gdouble *cmatrix;
    gint cmatrix_len;
    gchar *filter = self->filter;
    gboolean force_iir = filter && !strcmp (filter, "iir");
    gboolean force_fir = filter && !strcmp (filter, "fir");

    if ((force_iir || self->std_dev_x > 1.0) && !force_fir)
      {
        iir_young_find_constants (self->std_dev_x, &B, b);
        iir_young_hor_blur (input, temp,   B, b);
      }
    else
      {
        cmatrix_len =
            fir_gen_convolve_matrix (self->std_dev_x, &cmatrix);
        fir_hor_blur (input, temp, cmatrix, cmatrix_len);
        g_free (cmatrix);
      }
    if ((force_iir || self->std_dev_y > 1.0) && !force_fir)
      {
        iir_young_find_constants (self->std_dev_y, &B, b);
        iir_young_ver_blur (temp, output, B, b, self->std_dev_x * RADIUS_SCALE);
      }
    else
      {
        cmatrix_len =
            fir_gen_convolve_matrix (self->std_dev_y, &cmatrix);
        fir_ver_blur (temp, output, cmatrix, cmatrix_len, 
         self->std_dev_x * RADIUS_SCALE, self->std_dev_y * RADIUS_SCALE);
        g_free (cmatrix);
      }
  }

  gegl_buffer_destroy (input);
  gegl_buffer_destroy (temp);
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

  buf = g_malloc0 (gegl_buffer_get_pixel_count (src) * 4 * 4);
  w   = g_malloc0 (gegl_buffer_get_width (src) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RaGaBaA float"), buf, GEGL_AUTO_ROWSTRIDE);

  w_len = gegl_buffer_get_width (src);
  for (v=0; v<gegl_buffer_get_height (src); v++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             v*gegl_buffer_get_width (src)*4+c,
                             4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set (dst, NULL, babl_format ("RaGaBaA float"), buf);
  g_free (buf);
  g_free (w);
}

static void
iir_young_ver_blur (GeglBuffer *src,
                    GeglBuffer *dst,
                    gdouble     B,
                    gdouble    *b,
                    gint        offset)
{
  gint v;
  gint c;
  gint w_len;
  gfloat *buf;
  gfloat *w;

  buf = g_malloc0 (gegl_buffer_get_width (src) * gegl_buffer_get_height (src) * 4 * 4);
  w   = g_malloc0 (gegl_buffer_get_height (src) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RaGaBaA float"), buf, GEGL_AUTO_ROWSTRIDE);

  w_len = gegl_buffer_get_height (src);

  for (v=0; v<gegl_buffer_get_width (dst); v++)
    {
      for (c = 0; c < 4; c++)
        {
          iir_young_blur_1D (buf,
                             (v+offset)*4 + c,
                             gegl_buffer_get_width (src)*4,
                             B,
                             b,
                             w,
                             w_len);
        }
    }

  gegl_buffer_set (dst, gegl_buffer_get_extent (src), babl_format ("RaGaBaA float"), buf);
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

  src_buf = g_malloc0 (gegl_buffer_get_width (src) * gegl_buffer_get_height (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_get_width (dst) * gegl_buffer_get_height (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (v=0; v<gegl_buffer_get_height (dst); v++)
    for (u=0; u<gegl_buffer_get_width (dst); u++)
      {
        gint i;

        for (i=0; i<4; i++)
          dst_buf [offset++] = fir_get_mean_component (src_buf,
                                                       gegl_buffer_get_width (src),
                                                       gegl_buffer_get_height (src),
                                                       u - (matrix_length/2),
                                                       v,
                                                       matrix_length,
                                                       1,
                                                       i,
                                                       cmatrix);
      }

  gegl_buffer_set (dst, NULL, babl_format ("RaGaBaA float"), dst_buf);
  g_free (src_buf);
  g_free (dst_buf);
}

void
fir_ver_blur (GeglBuffer *src,
              GeglBuffer *dst,
              gdouble    *cmatrix,
              gint        matrix_length,
              gint        skipoffsetX,
              gint        skipoffsetY)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (gegl_buffer_get_width (src) * gegl_buffer_get_height (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_get_width (dst) * gegl_buffer_get_height (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset=0;
  for (v=0; v<gegl_buffer_get_height (dst); v++)
    for (u=0; u<gegl_buffer_get_width (dst); u++)
      {
        gint c;

        for (c=0; c<4; c++)
          dst_buf [offset++] = fir_get_mean_component (src_buf,
                                                       gegl_buffer_get_width (src),
                                                       gegl_buffer_get_height (src),
                                                       u + skipoffsetX,
                                                       v - (matrix_length/2) + skipoffsetY,
                                                       1,
                                                       matrix_length,
                                                       c,
                                                       cmatrix);
      }


  gegl_buffer_set (dst, NULL, babl_format ("RaGaBaA float"), dst_buf);
  g_free (src_buf);
  g_free (dst_buf);
}

#include <math.h>

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantOperation      *blur = GEGL_CHANT_OPERATION (operation);
  area->left = area->right = ceil (blur->std_dev_x * RADIUS_SCALE);
  area->top = area->bottom = ceil (blur->std_dev_y * RADIUS_SCALE);
}

#endif
