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
 
gegl_chant_double (radius, 0.0, 200.0, 4.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            box_blur
#define GEGL_CHANT_DESCRIPTION     "Performs an averaging of a square box of pixels."
#define GEGL_CHANT_SELF            "box-blur.c"
#define GEGL_CHANT_CATEGORIES      "blur"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

static void hor_blur (GeglBuffer *src,
                      GeglBuffer *dst,
                      gint        radius);

static void ver_blur (GeglBuffer *src,
                      GeglBuffer *dst,
                      gint        radius);

static GeglRect get_source_rect (GeglOperation *self);

#include <stdio.h>

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
      GeglRect   *result = gegl_operation_result_rect (operation);
      GeglRect    need   = get_source_rect (operation);
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
                                 "x",      need.x,
                                 "y",      need.y,
                                 "width",  need.w,
                                 "height", need.h,
                                 NULL);
          temp   = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RaGaBaA float"),
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

          hor_blur (temp_in, temp,  self->radius);
          ver_blur (temp, output, self->radius);
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
        filter->output = cropped;
        g_object_unref (output);
      }
    }
  return  TRUE;
}

static inline float
get_mean_component (gfloat *buf,
                    gint    buf_width,
                    gint    buf_height,
                    gint    x0,
                    gint    y0,
                    gint    width,
                    gint    height,
                    gint    component)
{
  gint    x, y;
  gdouble acc=0;
  gint    count=0;

  gint offset = (y0 * buf_width + x0) * 4 + component;

  for (y=y0; y<y0+height; y++)
    {
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            acc += buf [offset];
            count++;
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
   if (count)
     return acc/count;
   return 0.0;
}

static void
hor_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
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
                               u - radius,
                               v,
                               1 + radius*2,
                               1,
                               i);
      }

  gegl_buffer_set_fmt (dst, dst_buf, babl_format ("RaGaBaA float"));
  g_free (src_buf);
  g_free (dst_buf);
}


static void
ver_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
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
                               v - radius,
                               1,
                               1 + radius * 2,
                               c);
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
  gint       radius = ceil(blur->radius);

  if (!in_rect)
    return result;

  result = *in_rect;
  result.x-=radius;
  result.y-=radius;
  result.w+=radius*2;
  result.h+=radius*2;
  
  return result;
}

static GeglRect get_source_rect (GeglOperation *self)
{
  GeglChantOperation *blur   = GEGL_CHANT_OPERATION (self);
  GeglRect            rect;
  gint                radius;
 
  radius = ceil(blur->radius);

  rect  = *gegl_operation_get_requested_region (self);
  rect.x -= radius;
  rect.y -= radius;
  rect.w += radius*2;
  rect.h += radius*2;

  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  GeglRect need = get_source_rect (self);

  gegl_operation_set_source_region (self, "input", &need);

  return TRUE;
}

static GeglRect
get_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRect       region)
{
  GeglChantOperation *blur   = GEGL_CHANT_OPERATION (self);
  gint                radius;
 
  radius = ceil(blur->radius);

  region.x -= radius;
  region.y -= radius;
  region.w += radius*2;
  region.h += radius*2;
  return region;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region  = get_defined_region;
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
