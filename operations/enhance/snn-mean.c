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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (radius, 0.0, 70.0, 8.0,
  "Radius of square pixel region, (width and height will be radius*2+1.")
gegl_chant_int (pairs, 1, 2, 2, "Number of pairs higher number preserves more acute features")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "snn-mean.c"

#include "gegl-chant.h"
#include <math.h>

static void
snn_mean (GeglBuffer *src,
          GeglBuffer *dst,
          gdouble     radius,
          gint        pairs);


static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = ceil (o->radius);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO          *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer          *temp_in;
  GeglRectangle        compute;

  compute  = gegl_operation_compute_input_request (operation,
                                                   "input", result);

  if (o->radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);

      snn_mean (temp_in, output, o->radius, o->pairs);
      g_object_unref (temp_in);
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

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;

  for (y=0; y<gegl_buffer_get_height (dst); y++)
    for (x=0; x<gegl_buffer_get_width (dst); x++)
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
                      if (xs[i] >= 0 && xs[i] < gegl_buffer_get_width (src) &&
                          ys[i] >= 0 && ys[i] < gegl_buffer_get_height (src))
                        {
                          gfloat *tpix = src_buf + (xs[i]+ys[i]*gegl_buffer_get_width (src))*4;
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
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;

  operation_class->name        = "snn-mean";
  operation_class->categories  = "misc";
  operation_class->description =
        "Noise reducing edge enhancing blur filter based on Symmetric Nearest Neighbours";
}

#endif
