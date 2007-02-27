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
#define MAX_SAMPLES 20000 /* adapted to mean level of radius */

gegl_chant_double (radius, 0.0, 70.0, 8.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")
gegl_chant_int (pairs, 1, 2, 2, "Number of pairs higher number preserves more acute features")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME        snn_mean
#define GEGL_CHANT_DESCRIPTION "Noise reducing edge enhancing blur filter based on Symmetric Nearest Neighbours"
#define GEGL_CHANT_SELF        "snn-mean.c"
#define GEGL_CHANT_CATEGORIES  "misc"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"
#include <math.h>

static void
snn_mean (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     radius,
          gint        pairs);

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

          snn_mean (temp_in, output, self->radius, self->pairs);
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

#define RGB_LUMINANCE_RED    (0.212671)
#define RGB_LUMINANCE_GREEN  (0.715160)
#define RGB_LUMINANCE_BLUE   (0.072169)

static inline gfloat rgb2luminance (gfloat *pix)
{
  return pix[0] * RGB_LUMINANCE_RED +
         pix[1] * RGB_LUMINANCE_GREEN +
         pix[2] * RGB_LUMINANCE_BLUE;
}

#define POW2(a)((a)*(a))

static inline gfloat colordiff (gfloat *pixA,
                                gfloat *pixB)
{
  return POW2(pixA[0]-pixB[0])+
         POW2(pixA[1]-pixB[1])+
         POW2(pixA[2]-pixB[2]);
}


static void
snn_mean (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     radius,
          gint        pairs)
{
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
        gfloat *center_pix = src_buf + offset * 4;
        gfloat  accumulated[4]={0,};
        gint    count=0;
       
        /* iterate through the upper left quater of pixels */ 
        for (v=-radius;v<=0;v++)
          for (u=-radius;u<= (pairs==1?radius:0);u++)
            {
              gfloat *selected_pix = center_pix;
              gfloat  best_diff = 1000.0;
              gint    i;

              /* skip computations for the center pixel */
              if (u != 0 &&
                  v != 0)
                {
                  /* compute the coordinates of the symmetric pairs for
                   * this locaiton in the quadrant
                   */
                  gint xs[4] = {x+u, x-u, x-u, x+u};
                  gint ys[4] = {y+v, y-v, y+v, y-v};

                  /* check which member of the symmetric quadruple to use */
                  for (i=0;i<pairs*2;i++)
                    {
                      if (xs[i] >= 0 && xs[i] < src->width &&
                          ys[i] >= 0 && ys[i] < src->height)
                        {
                          gfloat *tpix = src_buf + (xs[i]+ys[i]*src->width)*4;
                          gfloat diff = colordiff (tpix, center_pix);
                          if (diff < best_diff)
                            {
                              best_diff = diff;
                              selected_pix = tpix;
                            }
                        }
                    }
                }

              /* accumulate the components of the best sample from
               * the symmetric quadruple
               */
              for (i=0;i<4;i++)
                {
                  accumulated[i] += selected_pix[i];
                }  
              count++;

              if (u==0 && v==0)
                break; /* to avoid doubly processing when using only 1 pair */
            }
        for (u=0; u<4; u++)
          dst_buf[offset*4+u] = accumulated[u]/count;
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
