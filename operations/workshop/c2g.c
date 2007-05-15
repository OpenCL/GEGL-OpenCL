/* A spatial color to greyscale converter.
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
 * Copyright 2007 Øyvind Kolås <oeyvindk@hig.no>
 *                Ivar Farup   <ivarf@hig.no>
 */

#if GEGL_CHANT_PROPERTIES 

gegl_chant_int (radius,     2, 5000.0, 384, "neighbourhood taken into account")
gegl_chant_int (samples,    0, 1000,    3,  "number of samples to do")
gegl_chant_int (iterations, 0, 1000.0, 23, "number of iterations (length of exposure)")
gegl_chant_double (strength,  -8, 8,  0.5, "how much the local optimum separation should be taken into account.")
#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME         c2g
#define GEGL_CHANT_DESCRIPTION  "Color to grayscale conversion that uses, spatial color differences to perform local grayscale contrast enhancement."
#define GEGL_CHANT_SELF         "c2g.c"
#define GEGL_CHANT_CATEGORIES   "enhance"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"
#include <math.h>
#define sq(a) ((a)*(a))

static void c2g (GeglBuffer *src,
                 GeglBuffer *dst,
                 gint        radius,
                 gint        samples,
                 gint        iterations,
                 gfloat     strength);

static GeglRectangle get_source_rect (GeglOperation *self,
                                      gpointer       context_id);

static void compute_luts(void);
#include <stdlib.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *output;


  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);

  compute_luts ();

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  {
    GeglRectangle   *result = gegl_operation_result_rect (operation, context_id);
    GeglRectangle    need   = *result;
    GeglBuffer      *temp_in;

    need.x-=self->radius;
    need.y-=self->radius;
    need.width +=self->radius*2;
    need.height +=self->radius*2;

    if (result->width == 0 ||
        result->height== 0 ||
        self->radius < 1.0)
      {
        output = g_object_ref (input);
      }
    else
      {
        temp_in = g_object_new (GEGL_TYPE_BUFFER,
                               "source", input,
                               "x",      need.x,
                               "y",      need.y,
                               "width",  need.width,
                               "height", need.height,
                               NULL);

        output = g_object_new (GEGL_TYPE_BUFFER,
                               "format", babl_format ("RGBA float"),
                               "x",      need.x,
                               "y",      need.y,
                               "width",  need.width,
                               "height", need.height,
                               NULL);

        c2g (temp_in, output, self->radius, self->samples, self->iterations, self->strength);
        g_object_unref (temp_in);
      }

    {
      GeglBuffer *cropped = g_object_new (GEGL_TYPE_BUFFER,
                                          "source", output,
                                          "x",      result->x,
                                          "y",      result->y,
                                          "width",  result->width ,
                                          "height", result->height,
                                          NULL);
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (cropped));
      g_object_unref (output);
    }
  }
  return  TRUE;
}

#define ANGLE_PRIME  95273
#define RADIUS_PRIME 29537
#define SCALE_PRIME 85643

static gfloat   lut_cos[ANGLE_PRIME];
static gfloat   lut_sin[ANGLE_PRIME];
static gfloat   radiuses[RADIUS_PRIME];
static gboolean luts_computed = FALSE; 
static gint     angle_no=0;
static gint     radius_no=0;

static void compute_luts(void)
{
  gint i;
  if (luts_computed)
    return;
  luts_computed = TRUE;

  for (i=0;i<ANGLE_PRIME;i++)
    {
      gfloat angle = (random() / (RAND_MAX*1.0)) * 3.141592653589793*2;
      lut_cos[i] = cos(angle);
      lut_sin[i] = sin(angle);
    }
  for (i=0;i<RADIUS_PRIME;i++)
    {
      radiuses[i] = (random() / (RAND_MAX*1.0));
    }
}

static inline void
sample (gfloat *buf,
        gint    width,
        gint    height,
        gint    x,
        gint    y,
        gfloat *dst)
{
  gfloat *pixel = buf + ((width * y) + x) * 4;
  gint c;

  for (c=0;c<4;c++)
    {
      dst[c]=pixel[c];
    }
}

static inline void
sample_min_max (gfloat *buf,
                gfloat *center_pix,
                gint    width,
                gint    height,
                gint    x,
                gint    y,
                gint    radius,
                gint    samples,
                gfloat *min,
                gfloat *max)
{
  gfloat best_min[3];
  gfloat best_max[3];

  gint i, c;

  for (c=0;c<3;c++)
    {
      best_min[c]=center_pix[c];
      best_max[c]=center_pix[c];
    }

  for (i=0; i<samples; i++)
    {
      gint u, v;
      gint angle = angle_no++;
      gfloat rmag = radiuses[radius_no++] * radius;

      if (angle_no>=ANGLE_PRIME)
        angle_no=0;
      if (radius_no>=RADIUS_PRIME)
        radius_no=0;

      u = x + rmag * lut_cos[angle];
      v = y + rmag * lut_sin[angle];

      if (u>=width)
        u=width-1;
      if (u<0)
        u=0;
      if (v>=height)
        v=height-1;
      if (v<0)
        v=0;

      {
        gfloat pixel[4];

        sample (buf, width, height, u, v, pixel);

        for (c=0;c<3;c++)
          {
            if (pixel[3]!=0.0 &&
                pixel[c]<best_min[c])
              best_min[c]=pixel[c];

            if (pixel[3]!=0.0 &&
                pixel[c]>best_max[c])
              best_max[c]=pixel[c];
          }
      }
    }
  for (c=0;c<3;c++)
    {
      min[c]=best_min[c];
      max[c]=best_max[c];
    }
}

static gfloat colordiff (gfloat *a,
                         gfloat *b)
{
  return sqrt(sq(a[0]-b[0])+
              sq(a[1]-b[1])+
              sq(a[2]-b[2]));
}


static void compute_envelopes (gfloat *buf,
                               gfloat *center_pixel,
                               gint    width,
                               gint    height,
                               gint    x,
                               gint    y,
                               gint    radius,
                               gint    samples,
                               gint    iterations,
                               gfloat *min_envelope,
                               gfloat *max_envelope)
{
  gint    i;
  gint    c;

  for (i=0;i<iterations;i++)
    {
      gfloat min[3];
      gfloat max[3];
      gfloat alpha = (i/(i+1.0));

      sample_min_max (buf,
                      center_pixel,
                      width,
                      height,
                      x, y,
                      radius, samples,
                      min, max);

      for (c=0;c<3;c++)
        {
          min_envelope[c] *= alpha;
          min_envelope[c] += min[c] * (1.0-alpha);

          max_envelope[c] *= alpha;
          max_envelope[c] += max[c] * (1.0-alpha);
        }
    }
}

static void c2g (GeglBuffer *src,
                 GeglBuffer *dst,
                 gint        radius,
                 gint        samples,
                 gint        iterations,
                 gfloat     strength)
{
  gint x,y;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (src->width * src->height * 4 * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 4);

  gegl_buffer_get (src, NULL, 1.0, babl_format ("RGBA float"), src_buf);
  for (y=radius; y<dst->height-radius; y++)
    {
      gint offset = ((src->width*y)+radius)*4;
      for (x=radius; x<dst->width-radius; x++)
        {
          gfloat *pixel = src_buf + offset;
          gfloat  min_envelope[3];
          gfloat  max_envelope[3];

          compute_envelopes (src_buf,
                             pixel,
                             src->width,
                             src->height,
                             x, y,
                             radius, samples,
                             iterations,
                             min_envelope, max_envelope);

          { /* now having a local blackpoint and a local white point
               we just wonder whether we are more black than white */
            gfloat gray;
            gint   c;
          
            gray = pixel[0]*0.212671 + pixel[1] * 0.715160 + pixel[2] * 0.072169;
#if 0 
          {
            gfloat black_dist;
            gfloat white_dist;
            white_dist = colordiff (pixel, max_envelope);
            black_dist = colordiff (pixel, min_envelope);

            if (black_dist+white_dist>0.001)
              {
                gray *= (1.0-strength);
                gray += strength * (black_dist / (black_dist + white_dist));
              }
          }
#endif

          {
            gfloat teller = 0;
            gfloat nevner = 0; 
            for (c=0; c<3; c++)
              {
                gfloat delta = max_envelope[c]-min_envelope[c];
                teller += (pixel[c] - min_envelope[c]) * delta;
                nevner += delta*delta;
              }

            if (nevner>0.000) /* if we found a range, modify the result */
              {
                gray *= (1.0-strength);
                gray += strength * (teller/nevner);
              }
          }

            for (c=0; c<3;c++)
              dst_buf[offset+c] = gray;
            dst_buf[offset+c] = pixel[c]; /* alpha is unmodified */
          }
          offset+=4;
        }
    }
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf);

  g_free (src_buf);
  g_free (dst_buf);
}
              
static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                     "input");

  GeglChantOperation *blur = GEGL_CHANT_OPERATION (operation);
  gint       radius = ceil(blur->radius);

  if (!in_rect)
    return result;

  result = *in_rect;
  if(0)if (result.width  != 0 &&
      result.height  != 0)
    {
      result.x-=radius;
      result.y-=radius;
      result.width +=radius*2;
      result.height +=radius*2;
    }
  
  return result;
}

static GeglRectangle get_source_rect (GeglOperation *self,
                                      gpointer       context_id)
{
  GeglChantOperation *blur   = GEGL_CHANT_OPERATION (self);
  GeglRectangle            rect;
  gint                radius;
 
  radius = ceil(blur->radius);

  rect  = *gegl_operation_get_requested_region (self, context_id);
  if (rect.width  != 0 &&
      rect.height  != 0)
    {
      rect.x -= radius;
      rect.y -= radius;
      rect.width  += radius*2;
      rect.height  += radius*2;
    }

  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       context_id)
{
  GeglRectangle need = get_source_rect (self, context_id);

  gegl_operation_set_source_region (self, context_id, "input", &need);

  return TRUE;
}

static GeglRectangle
get_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  GeglChantOperation *blur   = GEGL_CHANT_OPERATION (self);
  gint                radius;
 
  radius = ceil(blur->radius);

  region.x -= radius;
  region.y -= radius;
  region.width  += radius*2;
  region.height  += radius*2;
  return region;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region  = get_defined_region;
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
