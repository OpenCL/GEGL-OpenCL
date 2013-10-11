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
 * Copyright 1997 Miles O'Neal <meo@rru.com>  http://www.rru.com/~meo/
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

/*
 *  PICK Operation
 *     We pick a pixel at random from the neighborhood of the current pixel.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_seed   (seed, _("Seed"),
                   _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),
                   0.0, 100.0, 50.0,
                   _("Randomization"))

gegl_chant_int    (repeat, _("Repeat"),
                   1, 100, 1,
                   _("Repeat"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "noise-pick.c"

#include "gegl-chant.h"


#define CHUNK_SIZE 1024
#define MAX_HW_EXT 1224
#define SQR(x)     ((x)*(x))

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->repeat;

  gegl_operation_set_format (operation, "input" , babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
iterate (GeglRectangle *buffers_roi,
         GeglRectangle *current_roi,
         gfloat        *src,
         gfloat        *dst,
         gint           seed,
         gfloat         pct_random,
         gint           level)
{
  gint          rowstride;
  gint          x, y, x_start, y_start;
  GeglRectangle next_roi;

  if (level < 1)
    return;

  rowstride = buffers_roi->width;

  x_start = current_roi->x - buffers_roi->x;
  y_start = current_roi->y - buffers_roi->y;

  for(y = y_start; y < current_roi->height + y_start; y++)
    for(x = x_start; x < current_roi->width + x_start; x++)
      {
        gint   index_src, index_dst, b;
        gint   pos_x = buffers_roi->x + x;
        gint   pos_y = buffers_roi->y + y;
        gint   x2 = x;
        gint   y2 = y;
        gfloat rand;

        rand = gegl_random_float_range (seed, pos_x, pos_y, 0, level, 0.0, 100.0);

        if (rand <= pct_random)
          {
            gint k = gegl_random_int_range (seed, pos_x, pos_y, 0, level, 0, 9);

            x2 += (k % 3) - 1;
            y2 += (k / 3) - 1;
          }

        index_src = (y2 * rowstride + x2) * 4;
        index_dst = (y  * rowstride + x)  * 4;

        for (b = 0; b < 4; b++)
          dst[index_dst + b] = src[index_src + b];
      }

  next_roi = *GEGL_RECTANGLE (current_roi->x + 1,
                              current_roi->y + 1,
                              current_roi->width  - 2,
                              current_roi->height - 2);

  iterate (buffers_roi, &next_roi, dst, src, seed, pct_random, level - 1);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            src_rect, init_rect, chunked_result;
  gfloat                  *buf1, *buf2, *dst_buf;
  gint                     rowstride, start_x, start_y;
  gint                     i, j;

  buf1 = g_new (gfloat, SQR (MAX_HW_EXT) * 4);
  buf2 = g_new (gfloat, SQR (MAX_HW_EXT) * 4);

  for (j = 0; (j-1) * CHUNK_SIZE < result->height; j++)
    for (i = 0; (i-1) * CHUNK_SIZE < result->width; i++)
      {
        chunked_result = *GEGL_RECTANGLE (result->x + i * CHUNK_SIZE,
                                          result->y + j * CHUNK_SIZE,
                                          CHUNK_SIZE, CHUNK_SIZE);

        gegl_rectangle_intersect (&chunked_result, &chunked_result, result);

        if (chunked_result.width < 1  || chunked_result.height < 1)
          continue;

        src_rect.x      = chunked_result.x - op_area->left;
        src_rect.y      = chunked_result.y - op_area->top;
        src_rect.width  = chunked_result.width + op_area->left + op_area->right;
        src_rect.height = chunked_result.height + op_area->top + op_area->bottom;

        init_rect.x      = src_rect.x + 1;
        init_rect.y      = src_rect.y + 1;
        init_rect.width  = src_rect.width - 2;
        init_rect.height = src_rect.height - 2;

        gegl_buffer_get (input, &src_rect, 1.0,
                         babl_format ("RGBA float"), buf1,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

        iterate (&src_rect, &init_rect, buf1, buf2, o->seed,
                 o->pct_random, o->repeat);

        rowstride = src_rect.width;

        start_x = o->repeat;
        start_y = o->repeat;

        if (o->repeat % 2)
          dst_buf = buf2;
        else
          dst_buf = buf1;

        dst_buf += (start_y * rowstride + start_x) * 4;

        gegl_buffer_set (output, &chunked_result, 1.0,
                         babl_format ("RGBA float"), dst_buf,
                         rowstride * 4 * sizeof (gfloat));
      }

  g_free (buf1);
  g_free (buf2);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-pick",
    "categories", "noise",
    "description", _("Randomly interchange some pixels with neighbors"),
    NULL);
}

#endif
