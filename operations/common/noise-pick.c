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
 * Copyright 2013 Téo Mazars   <teo.mazars@ensimag.fr>
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
  const Babl              *format;

  op_area->left   =
  op_area->right  =
  op_area->top    =
  op_area->bottom = o->repeat;

  format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
iterate (GeglRectangle *current_roi,
         gint           src_rowstride,
         gchar         *src,
         gchar         *dst,
         gint           seed,
         gfloat         pct_random,
         gint           bpp,
         gint           repeat)
{
  gint x, y;

  for(y = 0; y < current_roi->height; y++)
    for(x = 0; x < current_roi->width; x++)
      {
        gint pos_x = current_roi->x + x;
        gint pos_y = current_roi->y + y;
        gint i;

        for (i = 0; i < repeat; i++)
          {
            gfloat rand;

            rand = gegl_random_float_range (seed, pos_x, pos_y, 0, i, 0.0, 100.0);

            if (rand <= pct_random)
              {
                gint k = gegl_random_int_range (seed, pos_x, pos_y, 0, i, 0, 9);

                pos_x += (k % 3) - 1;
                pos_y += (k / 3) - 1;
              }
          }

        pos_x -= current_roi->x;
        pos_y -= current_roi->y;

        memcpy (dst + (x + y * current_roi->width) * bpp,
                src + (pos_x + pos_y * src_rowstride) * bpp,
                bpp);
      }
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
  GeglRectangle            src_rect, chunked_result;
  gchar                   *input_buf, *output_buf;
  gchar                   *src_buf;
  gint                     rowstride;
  gint                     i, j;
  const Babl              *format;
  gint                     bpp;

  format = gegl_operation_get_source_format (operation, "input");
  bpp = babl_format_get_bytes_per_pixel (format);

  input_buf  = g_malloc (SQR (MAX_HW_EXT) * bpp);
  output_buf = g_malloc (SQR (CHUNK_SIZE) * bpp);

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

        gegl_buffer_get (input, &src_rect, 1.0, format, input_buf,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

        rowstride = src_rect.width;
        src_buf = (input_buf + (o->repeat * rowstride + o->repeat) * bpp);

        iterate (&chunked_result, rowstride, src_buf, output_buf,
                 o->seed, o->pct_random, bpp, o->repeat);

        gegl_buffer_set (output, &chunked_result, 1.0, format, output_buf,
                         GEGL_AUTO_ROWSTRIDE);
      }

  g_free (input_buf);
  g_free (output_buf);

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
