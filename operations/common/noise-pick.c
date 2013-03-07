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

gegl_chant_seed   (seed, _("Seed"), _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),
                   0.0, 100.0, 50.0, _("Randomization"))

gegl_chant_int    (repeat, _("Repeat"),
                   1, 100, 1, _("Repeat"))


#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "noise-pick.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left   = 1;
  op_area->right  = 1;
  op_area->top    = 1;
  op_area->bottom = 1;

  gegl_operation_set_format (operation, "input" , babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
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
  GeglBuffer              *tmp;
  gfloat                  *src_buf;
  gfloat                  *dst_buf;
  gfloat                  *in_pixel;
  gfloat                  *out_pixel;
  gint                     n_pixels = result->width * result->height;
  gint                     width    = result->width;
  GeglRectangle            src_rect;
  gint                     total_pixels;
  gint                     i;

  tmp = gegl_buffer_new (result, babl_format ("RGBA float"));

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_pixels = src_rect.height * src_rect.width;

  src_buf = g_slice_alloc (4 * total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (4 * n_pixels * sizeof (gfloat));

  gegl_buffer_copy (input, NULL, tmp, NULL);

  for (i = 0; i < o->repeat; i++)
    {
      gint x, y, n;

      x = result->x;
      y = result->y;
      n = 0;

      n_pixels = result->width * result->height;

      gegl_buffer_get (tmp, &src_rect, 1.0,
                       babl_format ("RGBA float"), src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      in_pixel  = src_buf + (src_rect.width + 1) * 4;
      out_pixel = dst_buf;

      while (n_pixels--)
        {
          gint b;

          if (gegl_random_double_range (o->seed, x, y, 0, n++, 0.0, 100.0) <=
              o->pct_random)
            {
              gint k = gegl_random_int_range (o->seed, x, y, 0, n++, 0, 9);

              for (b = 0; b < 4; b++)
                {
                  switch (k)
                    {
                    case 0:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4 - 4];
                      break;
                    case 1:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4];
                      break;
                    case 2:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4 + 4];
                      break;
                    case 3:
                      out_pixel[b] = in_pixel[b - 4];
                      break;
                    case 4:
                      out_pixel[b] = in_pixel[b];
                      break;
                    case 5:
                      out_pixel[b] = in_pixel[b + 4];
                      break;
                    case 6:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4 - 4];
                      break;
                    case 7:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4];
                      break;
                    case 8:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4 + 4];
                      break;
                    }
                }
            }
          else
            {
              for (b = 0; b < 4; b++)
                {
                  out_pixel[b] = in_pixel[b];
                }
            }

          if (n_pixels % width == 0)
            in_pixel += 12;
          else
            in_pixel += 4;

          out_pixel += 4;

          x++;
          if (x >= result->x + result->width)
            {
              x = result->x;
              y++;
            }
        }

      gegl_buffer_set (tmp, result, 0,
                       babl_format ("RGBA float"), dst_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gegl_buffer_copy (tmp, NULL, output, NULL);

  g_slice_free1 (4 * total_pixels * sizeof (gfloat), src_buf);
  g_slice_free1 (4 * n_pixels * sizeof (gfloat), dst_buf);

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
