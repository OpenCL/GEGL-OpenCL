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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */
#if GEGL_CHANT_PROPERTIES
 
#define MAX_SAMPLES 20000 /* adapted to max level of radius */

gegl_chant_double (radius, 0.0, 70.0, 4.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")
gegl_chant_double (percentile, 0.0, 100.0, 50, "The percentile to compute, defaults to 50, which is a median filter.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            disc_percentile
#define GEGL_CHANT_DESCRIPTION     "Sets the target pixel to the color corresponding to a given percentile when colors are sorted by luminance."
#define GEGL_CHANT_SELF            "disc-percentile.c"
#define GEGL_CHANT_CATEGORIES      "misc"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

static void median (GeglBuffer *src,
                    GeglBuffer *dst,
                    gint        radius,
                    gdouble     rank);

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

          median (temp_in, output, self->radius, self->percentile / 100.0);
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


typedef struct
{
  int       head;
  int       next[MAX_SAMPLES];
  float     luma[MAX_SAMPLES];
  float    *pixel[MAX_SAMPLES];
  int       items;
} RankList;

static void
list_clear (RankList * p)
{
  p->items = 0;
  p->next[0] = -1;
}

static inline void
list_add (RankList *p,
          gfloat    lumniosity,
          gfloat   *pixel)
{
  gint location;

  location = p->items;

  p->items++;
  p->luma[location] = lumniosity;
  p->pixel[location] = pixel;
  p->next[location] = -1;

  if (p->items == 1)
    {
      p->head = location;
      return;
    }
  if (lumniosity <= p->luma[p->head])
    {
      p->next[location] = p->head;
      p->head = location;
    }
  else
    {
      gint   prev, i;
      prev = p->head;
      i = prev;
      while (i >= 0 && p->luma[i] < lumniosity)
        {
          prev = i;
          i = p->next[i];
        }
      p->next[location] = p->next[prev];
      p->next[prev] = location;
    }
}

static inline gfloat *
list_percentile (RankList *p,
                 gdouble   percentile)
{
  gint       i = p->head;
  gint       pos = 0;
  if (!p->items)
    return NULL;
  if (percentile >= 1.0)
    percentile = 1.0;
  while (pos < p->items * percentile &&
         p->pixel[p->next[i]])
    {
      i = p->next[i];
      pos++;
    }
  return p->pixel[i];
}

static void
median (GeglBuffer *src,
        GeglBuffer *dst,
        gint        radius,
        gdouble     rank)
{
  RankList list = {0};

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;


  src_buf = g_malloc0 (src->width * src->height * 4 * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 4);

  gegl_buffer_get (src, NULL, 1.0, babl_format ("RGBA float"), src_buf);

  offset = 0;
  for (y=0; y<dst->height; y++)
    for (x=0; x<dst->width; x++)
      {
        gint u,v;
        gfloat *median_pix;
        
        list_clear (&list);

        for (v=y-radius;v<=y+radius;v++)
          for (u=x-radius;u<=x+radius;u++)
            {
              gint ru, rv;
              
              ru = (x-u)*(x-u);
              rv = (y-v)*(y-v);

              if (u >= 0 && u < dst->width &&
                  v >= 0 && v < dst->height &&
                  (ru+rv) < radius* radius
                  )
                {
                  gfloat *src_pix = src_buf + (u+(v * src->width)) * 4;
                  gfloat luma = (src_pix[0] * 0.212671 +
                                 src_pix[1] * 0.715160 +
                                 src_pix[2] * 0.072169);
                  list_add (&list, luma, src_pix);
                }
            }

        median_pix = list_percentile (&list, rank);
        for (u=0; u<4;u++)
          dst_buf[offset*4+u] = median_pix[u];
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
