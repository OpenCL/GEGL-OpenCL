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

gegl_chant_boolean (horizontal,  _("Horizontal"),  TRUE,
                    _("Horizontal"))

gegl_chant_boolean (vertical,  _("Vertical"),  TRUE,
                    _("Vertical"))

gegl_chant_boolean (keep_signal,  _("Keep Signal"),  TRUE,
                    _("Keep Signal"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "edge-sobel.c"

#include "gegl-chant.h"
#include <math.h>

#define SOBEL_RADIUS 1

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal);

#include <stdio.h>

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  //GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = SOBEL_RADIUS;
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;

  compute = gegl_operation_get_required_for_output (operation, "input",result);

  edge_sobel (input, &compute, output, result, o->horizontal, o->vertical, o->keep_signal);

  return  TRUE;
}

inline static gfloat
RMS(gfloat a, gfloat b)
{
  return sqrtf(a*a+b*b);
}

static void
edge_sobel (GeglBuffer          *src,
            const GeglRectangle *src_rect,
            GeglBuffer          *dst,
            const GeglRectangle *dst_rect,
            gboolean            horizontal,
            gboolean            vertical,
            gboolean            keep_signal)
{

  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  gint src_width = src_rect->width;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {

        gfloat hor_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat ver_grad[3] = {0.0f, 0.0f, 0.0f};
        gfloat gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        gfloat *center_pix = src_buf + ((x+SOBEL_RADIUS)+((y+SOBEL_RADIUS) * src_width)) * 4;

        gint c;

        if (horizontal)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                hor_grad[c] +=
                    -1.0f*src_pix[c-4-src_width*4]+ src_pix[c+4-src_width*4] +
                    -2.0f*src_pix[c-4] + 2.0f*src_pix[c+4] +
                    -1.0f*src_pix[c-4+src_width*4]+ src_pix[c+4+src_width*4];
          }

        if (vertical)
          {
            gint i=x+SOBEL_RADIUS, j=y+SOBEL_RADIUS;
            gfloat *src_pix = src_buf + (i + j * src_width) * 4;

            for (c=0;c<3;c++)
                ver_grad[c] +=
                  -1.0f*src_pix[c-4-src_width*4]-2.0f*src_pix[c-src_width*4]-1.0f*src_pix[c+4-src_width*4] +
                  src_pix[c-4+src_width*4]+2.0f*src_pix[c+src_width*4]+     src_pix[c+4+src_width*4];
        }

        if (horizontal && vertical)
          {
            for (c=0;c<3;c++)
              // normalization to [0, 1]
              gradient[c] = RMS(hor_grad[c],ver_grad[c])/1.41f;
          }
        else
          {
            if (keep_signal)
              {
                for (c=0;c<3;c++)
                  gradient[c] = hor_grad[c]+ver_grad[c];
              }
            else
              {
                for (c=0;c<3;c++)
                  gradient[c] = fabsf(hor_grad[c]+ver_grad[c]);
              }
          }

        //alpha
        gradient[3] = center_pix[3];

        for (c=0; c<4;c++)
          dst_buf[offset*4+c] = gradient[c];

        offset++;
      }

  gegl_buffer_set (dst, dst_rect, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
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

  operation_class->name        = "gegl:edge-sobel";
  operation_class->categories  = "edge-detect";
  operation_class->description =
        _("Specialized direction-dependent edge detection");
}

#endif
