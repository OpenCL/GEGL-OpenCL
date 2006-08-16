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
 */

#ifdef CHANT_SELF
 
chant_double (radius_x, 0.0, 200.0, 4.0, "blur radius in horizontal direction.")
chant_double (radius_y, 0.0, 200.0, 4.0, "blur radius in vertical direction.")

#else

#define CHANT_FILTER
#define CHANT_NAME            gaussian_blur
#define CHANT_DESCRIPTION     "Performs an averaging of neighbouring pixels with the normal distribution as weighting."
#define CHANT_SELF            "gaussian-blur.c"
#define CHANT_CATEGORIES      "blur"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"

#include <math.h>
#include <stdio.h>

static void
hor_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     B,
          gdouble    *b);

static void
ver_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     B,
          gdouble    *b);

static void
find_iir_constants (gfloat   radius, 
                    gdouble *B,
                    gdouble *b);




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

          {
            gdouble B, b[4];

            find_iir_constants (self->radius_x, &B, b);
            hor_blur (temp_in, temp,   B, b);
            
            find_iir_constants (self->radius_y, &B, b);
            ver_blur (temp,    output, B, b);
          }
          
          g_object_unref (temp);
          g_object_unref (temp_in);
        }

      if (filter->output)
        g_object_unref (filter->output);
      filter->output = output;
    }
  return  TRUE;
}

static void
find_iir_constants (gfloat   radius, 
                    gdouble *B,
                    gdouble *b)
{
  gdouble sigma0, q;
  
  sigma0 = (radius/3.0)+0.5;

  if(sigma0 >= 2.5)
    q = 0.98711*sigma0 - 0.96330;
  else
    q = 3.97156 - 4.14554*sqrt(1-0.26891*sigma0);

  b[0] = 1.57825 + (2.44413*q) + (1.4281*q*q) + (0.422205*q*q*q);
  b[1] = (2.44413*q) + (2.85619*q*q) + (1.26661*q*q*q);
  b[2] = -((1.4281*q*q) + (1.26661*q*q*q));
  b[3] = 0.422205*q*q*q;

  *B = 1 - ( (b[1]+b[2]+b[3])/b[0] );
}

static inline void
blur_1D (gfloat  * buf,
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
hor_blur (GeglBuffer *src,
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
          blur_1D (buf,
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
ver_blur (GeglBuffer *src,
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
          blur_1D (buf,
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

static gboolean
calc_have_rect (GeglOperation *operation)
{
  GeglRect *in_rect = gegl_operation_get_have_rect (operation, "input");
  ChantInstance* self = CHANT_INSTANCE(operation);
  gint radius_x       = ceil(self->radius_x+0.5);
  gint radius_y       = ceil(self->radius_y+0.5);
  if(!in_rect)
    return FALSE;

  gegl_operation_set_have_rect (operation, 
     in_rect->x-radius_x,   in_rect->y-radius_y,
     in_rect->w+radius_x*2, in_rect->h+radius_y*2);
  return TRUE;
}


static gboolean
calc_need_rect (GeglOperation *self)
{
  GeglRect  *need   = gegl_operation_need_rect (self);
  ChantInstance *blur = CHANT_INSTANCE(self);
  gint radius_x = ceil(blur->radius_x+0.5);
  gint radius_y = ceil(blur->radius_y+0.5);

  gegl_operation_set_need_rect (self, "input",
     need->x-radius_x,   need->y-radius_y,
     need->w+radius_x*2, need->h+radius_y*2);
  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->calc_have_rect = calc_have_rect;
  operation_class->calc_need_rect = calc_need_rect;
}


#endif
