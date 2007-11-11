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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */
#if GEGL_CHANT_PROPERTIES
 
#define MAX_SAMPLES 20000 /* adapted to max level of radius */

gegl_chant_double (radius, 0.0, 70.0, 4.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")
gegl_chant_double (percentile, 0.0, 100.0, 50, "The percentile to compute, defaults to 50, which is a median filter.")

#else

#define GEGL_CHANT_NAME            disc_percentile
#define GEGL_CHANT_SELF            "disc-percentile.c"
#define GEGL_CHANT_DESCRIPTION     "Sets the target pixel to the color corresponding to a given percentile when colors are sorted by luminance."
#define GEGL_CHANT_CATEGORIES      "misc"

#define GEGL_CHANT_AREA_FILTER
#include "gegl-chant.h"
#include <math.h>

static void median (GeglBuffer *src,
                    GeglBuffer *dst,
                    gint        radius,
                    gdouble     rank);

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
      GeglBuffer      *temp_in;
      GeglRectangle    compute  = gegl_operation_compute_input_request (operation, "inputt", gegl_operation_need_rect (operation, context_id));

      if (self->radius < 1.0)
        {
          output = g_object_ref (input);
        }
      else
        {
          temp_in = gegl_buffer_create_sub_buffer (input, &compute);
          output = gegl_buffer_new (&compute, babl_format ("RGBA float"));

          median (temp_in, output, self->radius, self->percentile / 100.0);
          g_object_unref (temp_in);
        }

      {
        GeglBuffer *cropped = gegl_buffer_create_sub_buffer (output, result);
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


  src_buf = g_malloc0 (gegl_buffer_pixel_count (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_pixel_count (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (y=0; y<gegl_buffer_height (dst); y++)
    for (x=0; x<gegl_buffer_width (dst); x++)
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

              if (u >= 0 && u < gegl_buffer_width (dst) &&
                  v >= 0 && v < gegl_buffer_height (dst) &&
                  (ru+rv) < radius* radius
                  )
                {
                  gfloat *src_pix = src_buf + (u+(v * gegl_buffer_width (src))) * 4;
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

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantOperation      *filter = GEGL_CHANT_OPERATION (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (filter->radius);
}


#endif
