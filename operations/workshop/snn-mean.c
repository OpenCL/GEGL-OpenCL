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

#define GEGL_CHANT_NAME        snn_mean
#define GEGL_CHANT_SELF        "snn-mean.c"
#define GEGL_CHANT_DESCRIPTION "Noise reducing edge enhancing blur filter based on Symmetric Nearest Neighbours"
#define GEGL_CHANT_CATEGORIES  "misc"

#define GEGL_CHANT_AREA_FILTER

#include "gegl-chant.h"
#include <math.h>

static void
snn_mean (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     radius,
          gint        pairs);


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

          snn_mean (temp_in, output, self->radius, self->pairs);
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

  src_buf = g_malloc0 (gegl_buffer_pixel_count (src) * 4 * 4);
  dst_buf = g_malloc0 (gegl_buffer_pixel_count (dst) * 4 * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;

  for (y=0; y<gegl_buffer_height (dst); y++)
    for (x=0; x<gegl_buffer_width (dst); x++)
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
                      if (xs[i] >= 0 && xs[i] < gegl_buffer_width (src) &&
                          ys[i] >= 0 && ys[i] < gegl_buffer_height (src))
                        {
                          gfloat *tpix = src_buf + (xs[i]+ys[i]*gegl_buffer_width (src))*4;
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

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantOperation      *filter = GEGL_CHANT_OPERATION (operation);
  area->left = area->right = area->top = area->bottom =
  ceil (filter->radius);
}

#endif
