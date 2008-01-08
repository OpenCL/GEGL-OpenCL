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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
 
gegl_chant_double (radius, 0.0, 200.0, 4.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")

#else

#define GEGL_CHANT_NAME            box_blur
#define GEGL_CHANT_SELF            "box-blur.c"
#define GEGL_CHANT_DESCRIPTION     "Performs an averaging of a square box of pixels."
#define GEGL_CHANT_CATEGORIES      "blur"

#define GEGL_CHANT_AREA_FILTER

#include "gegl-chant.h"

static void hor_blur (GeglBuffer *src,
                      GeglBuffer *dst,
                      gint        radius);

static void ver_blur (GeglBuffer *src,
                      GeglBuffer *dst,
                      gint        radius);

#include <stdio.h>

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantOperation  *self;

  GeglBuffer *temp;

  self  = GEGL_CHANT_OPERATION (operation);

  temp  = gegl_buffer_new (gegl_buffer_get_extent (input),
                           babl_format ("RaGaBaA float"));

  hor_blur (input, temp,  self->radius);
  ver_blur (temp, output, self->radius);

  g_object_unref (temp);
  return  TRUE;
}

#ifdef DEAD
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
#endif

static void inline
get_mean_components (gfloat *buf,
                     gint    buf_width,
                     gint    buf_height,
                     gint    x0,
                     gint    y0,
                     gint    width,
                     gint    height,
                     gfloat *components)
{
  gint    y;
  gdouble acc[4]={0,0,0,0};
  gint    count[4]={0,0,0,0};

  gint offset = (y0 * buf_width + x0) * 4;

  for (y=y0; y<y0+height; y++)
    {
    gint x;
    for (x=x0; x<x0+width; x++)
      {
        if (x>=0 && x<buf_width &&
            y>=0 && y<buf_height)
          {
            gint c;
            for (c=0;c<4;c++)
              {
                acc[c] += buf [offset+c];
                count[c]++;
              }
          }
        offset+=4;
      }
      offset+= (buf_width * 4) - 4 * width;
    }
    {
      gint c;
      for (c=0;c<4;c++)
        {
         if (count[c])
           components[c] = acc[c]/count[c];
         else
           components[c] = 0.0;
        }
    }
}

/* expects src and dst buf to have the same extent */
static void
hor_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
{
  gint u,v;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  /* src == dst for hor blur */
  src_buf = g_malloc0 (gegl_buffer_get_pixel_count (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_get_pixel_count (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RaGaBaA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (v=0; v<gegl_buffer_get_height (dst); v++)
    for (u=0; u<gegl_buffer_get_width (dst); u++)
      {
        gint i;
        gfloat components[4];

        get_mean_components (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             u - radius,
                             v,
                             1 + radius*2,
                             1,
                             components);
        
        for (i=0; i<4; i++)
          dst_buf [offset++] = components[i];
      }

  gegl_buffer_set (dst, NULL, babl_format ("RaGaBaA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


/* expects dst buf to be radius smaller than src buf */
static void
ver_blur (GeglBuffer *src,
          GeglBuffer *dst,
          gint        radius)
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
        gfloat components[4];
        gint c;

        get_mean_components (src_buf,
                             gegl_buffer_get_width (src),
                             gegl_buffer_get_height (src),
                             u + radius,  /* 1x radius is the offset between the bufs */
                             v - radius + radius, /* 1x radius is the offset between the bufs */
                             1,
                             1 + radius * 2,
                             components);
        
        for (c=0; c<4; c++)
          dst_buf [offset++] = components[c];
      }

  gegl_buffer_set (dst, NULL, babl_format ("RaGaBaA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

#include <math.h>

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantOperation      *blur = GEGL_CHANT_OPERATION (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (blur->radius);
}


#endif
