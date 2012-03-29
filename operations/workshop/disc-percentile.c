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

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (radius, _("Radius"), 0.0, 70.0, 4.0,
  _("Radius of square pixel region (width and height will be radius*2+1)"))
gegl_chant_double (percentile, _("Percentile"), 0.0, 100.0, 50,
                   _("The percentile to compute, defaults to 50, which is a median filter."))

#else

#define MAX_SAMPLES 20000 /* adapted to max level of radius */

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "disc-percentile.c"

#include "gegl-chant.h"
#include <math.h>

static void median (GeglBuffer *src,
                      GeglBuffer *dst,
                      gint        radius,
                      gdouble     rank);

#include <stdio.h>

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


  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, NULL, 1.0, babl_format ("RGBA float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset = 0;
  for (y=0; y<gegl_buffer_get_height (dst); y++)
    for (x=0; x<gegl_buffer_get_width (dst); x++)
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

              if (u >= 0 && u < gegl_buffer_get_width (dst) &&
                  v >= 0 && v < gegl_buffer_get_height (dst) &&
                  (ru+rv) < radius* radius
                  )
                {
                  gfloat *src_pix = src_buf + (u+(v * gegl_buffer_get_width (src))) * 4;
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
  gegl_buffer_set (dst, NULL, 0, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);
  area->left = area->right = area->top = area->bottom = ceil (o->radius);
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *temp_in;
  GeglRectangle compute =
        gegl_operation_get_required_for_output (operation, "input", result);

  if (o->radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);

      median (temp_in, output, o->radius, o->percentile / 100.0);
      g_object_unref (temp_in);
    }

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:disc-percentile",
  "categories"  , "misc",
  "description" ,
        _("Sets the target pixel to the color corresponding to a given"
          " percentile when colors are sorted by luminance."),
        NULL);
}

#endif
