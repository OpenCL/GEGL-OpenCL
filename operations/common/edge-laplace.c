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
 */

/*
 * Copyright 2011 Victor Oliveira <victormatheus@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "edge-laplace.c"

#include "gegl-chant.h"
#include <math.h>

#define LAPLACE_RADIUS 1

static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect);

#include <stdio.h>

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  //GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = LAPLACE_RADIUS;
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglRectangle compute;

  compute = gegl_operation_get_required_for_output (operation, "input", result);

  edge_laplace (input, &compute, output, result);

  return  TRUE;
}

static void
minmax  (gfloat  x1,
         gfloat  x2,
         gfloat  x3,
         gfloat  x4,
         gfloat  x5,
         gfloat *min_result,
         gfloat *max_result)
{
  gfloat min1, min2, max1, max2;

  if (x1 > x2)
    {
      max1 = x1;
      min1 = x2;
    }
  else
    {
      max1 = x2;
      min1 = x1;
    }

  if (x3 > x4)
    {
      max2 = x3;
      min2 = x4;
    }
  else
    {
      max2 = x4;
      min2 = x3;
    }

  if (min1 < min2)
    *min_result = fminf (min1, x5);
  else
    *min_result = fminf (min2, x5);

  if (max1 > max2)
    *max_result = fmaxf (max1, x5);
  else
    *max_result = fmaxf (max2, x5);
}


static void
edge_laplace (GeglBuffer          *src,
              const GeglRectangle *src_rect,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect)
{

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *temp_buf;
  gfloat *dst_buf;

  gint src_width = src_rect->width;

  src_buf  = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  temp_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf  = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0,
                   babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {
        gfloat *src_pix;

        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gint c;

        gfloat minval, maxval;

        gint i=x+LAPLACE_RADIUS, j=y+LAPLACE_RADIUS;
        offset = i + j * src_width;
        src_pix = src_buf + offset * 4;

        for (c=0;c<3;c++)
          {
            minmax (src_pix[c-src_width*4], src_pix[c+src_width*4],
                    src_pix[c-4], src_pix[c+4], src_pix[c],
                    &minval, &maxval); /* four-neighbourhood */

            gradient[c] = 0.5f * fmaxf((maxval-src_pix[c]), (src_pix[c]-minval));

            gradient[c] = (src_pix[c-4-src_width*4] +
                           src_pix[c-src_width*4] +
                           src_pix[c+4-src_width*4] +

                           src_pix[c-4] -8.0f* src_pix[c] +src_pix[c+4] +

                           src_pix[c-4+src_width*4] + src_pix[c+src_width*4] +
                           src_pix[c+4+src_width*4]) > 0.0f?
                          gradient[c] : -1.0f*gradient[c];
        }

        //alpha
        gradient[3] = src_pix[3];

        for (c=0; c<4;c++)
          temp_buf[offset*4+c] = gradient[c];
      }

  //1-pixel edges
  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {

        gfloat value[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gint c;

        gint i=x+LAPLACE_RADIUS, j=y+LAPLACE_RADIUS;
        gfloat *src_pix = temp_buf + (i + j * src_width) * 4;

        for (c=0;c<3;c++)
        {
          gfloat current = src_pix[c];
          current = ((current > 0.0f) &&
                     (src_pix[c-4-src_width*4] < 0.0f ||
                      src_pix[c+4-src_width*4] < 0.0f ||
                      src_pix[c  -src_width*4] < 0.0f ||
                      src_pix[c-4+src_width*4] < 0.0f ||
                      src_pix[c+4+src_width*4] < 0.0f ||
                      src_pix[   +src_width*4] < 0.0f ||
                      src_pix[c-4            ] < 0.0f ||
                      src_pix[c+4            ] < 0.0f))?
                    current : 0.0f;

          value[c] = current;
        }

        //alpha
        value[3] = src_pix[3];

        for (c=0; c<4;c++)
          dst_buf[offset*4+c] = value[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (temp_buf);
  g_free (dst_buf);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:edge-laplace";
  operation_class->categories  = "edge-detect";
  operation_class->description =
        _("High-resolution edge detection");
}

#endif
