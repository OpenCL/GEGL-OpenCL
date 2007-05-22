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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
 
gegl_chant_double (radius, 0.0, 50.0, 4.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")

#else

#define GEGL_CHANT_NAME            kuwahara_min
#define GEGL_CHANT_SELF            "kuwahara-min.c"
#define GEGL_CHANT_DESCRIPTION     "Edge preserving min filter"
#define GEGL_CHANT_CATEGORIES      "misc"

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_CLASS_INIT

#include "gegl-chant.h"

static void
kuwahara (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius);

static GeglRectangle get_source_rect (GeglOperation *self,
                                      gpointer       context_id);

#include <stdio.h>

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


  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
    {
      GeglRectangle   *result = gegl_operation_result_rect (operation, context_id);
      GeglRectangle    work   = *result;
      GeglBuffer      *temp_in;

      work.x-=self->radius;
      work.y-=self->radius;
      work.width +=self->radius*2;
      work.height +=self->radius*2;

      
      temp_in = g_object_new (GEGL_TYPE_BUFFER,
                             "source", input,
                             "x",      work.x,
                             "y",      work.y,
                             "width",  work.width ,
                             "height", work.height ,
                             NULL);

      output = g_object_new (GEGL_TYPE_BUFFER,
                             "format", babl_format ("RGBA float"),
                             "x",      work.x,
                             "y",      work.y,
                             "width",  work.width ,
                             "height", work.height ,
                             NULL);

      kuwahara (temp_in, output, self->radius);
      g_object_unref (temp_in);
      

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

static inline void
compute_rectangle (gfloat *buf,
                   gint    buf_width,
                   gint    buf_height,
                   gint    x0,
                   gint    y0,
                   gint    width,
                   gint    height,
                   gint    component,
                   gfloat *pmin,
                   gfloat *pmax,
                   gfloat *pmean,
                   gfloat *pvariance)
{
  gint    x, y;
  gfloat  max   = -1000000000.0;
  gfloat  min   =  1000000000.0;
  gfloat  mean  =  0.0;
  gint    count =  0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            if (buf [offset] > max)
              max = buf[offset];
            if (buf [offset] < min)
              min = buf[offset];
            mean += buf[offset];
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
  if (pmin)
    *pmin = min;
  if (pmax)
    *pmax = max;
  if (pmean && count)
    *pmean = mean/count;
  if (pvariance)
    *pvariance = max-min;
}

static void
kuwahara (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (src->width * src->height * 4 * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 4);

  gegl_buffer_get (src, NULL, 1.0, babl_format ("RGBA float"), src_buf);

  offset = 0;
  for (v=0; v<dst->height; v++)
    for (u=0; u<dst->width; u++)
      {
        gint component;

        for (component=0; component<3; component++)
          {
            gfloat value=0.0;
            gfloat best=1000000.0;

            gfloat min = 0.0;
            gfloat variance = 0.0;

            compute_rectangle (src_buf,
                               src->width,
                               src->height,
                               u - radius - 1,
                               v - radius - 1,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL, 
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               src->width,
                               src->height,
                               u,
                               v - radius - 1,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL, 
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               src->width,
                               src->height,
                               u - radius - 1,
                               v,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL, 
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }

            compute_rectangle (src_buf,
                               src->width,
                               src->height,
                               u,
                               v,
                               1 + radius,
                               1 + radius,
                               component,
                               &min, /* min */
                               NULL, /* max */
                               NULL, 
                               &variance);
            if (variance<best)
              {
                value = min;
                best = variance;
              }
            dst_buf [offset++] = value;
          }
          dst_buf [offset] = src_buf[offset];
          offset++;
      }

  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf);
  g_free (src_buf);
  g_free (dst_buf);
}


#include <math.h>
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
  if (result.width  != 0 &&
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
