/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2009 Martin Nordholts
 */

#include <string.h>

#include "gegl.h"

#define RED      0
#define GREEN    1
#define BLUE     2

#define SUCCESS  0
#define FAILURE -1

#define RUNS     3


int main(int argc, char *argv[])
{
  int           result                    = SUCCESS;
  guchar        result_buffer[3]          = { 0, 0, 0 };
  int           i                         = 0;
  GeglNode     *color                     = NULL;
  GeglColor    *colors[RUNS]              = { 0, 0, 0 };
  guchar        expected_results[RUNS][3] = { { 255, 0,   0   },
                                              { 0,   255, 0   },
                                              { 0,   0,   255 } };
  GeglRectangle rois[RUNS]                = { { 0,                0,                1, 1 },
                                              { G_MININT / 2,     G_MININT / 2,     1, 1 },
                                              { G_MAXINT / 2 - 1, G_MAXINT / 2 - 1, 1, 1 } };

  /* Init */
  g_thread_init (NULL);
  gegl_init (&argc, &argv);
  colors[0] = gegl_color_new ("rgb(1.0, 0.0, 0.0)");
  colors[1] = gegl_color_new ("rgb(0.0, 1.0, 0.0)");
  colors[2] = gegl_color_new ("rgb(0.0, 0.0, 1.0)");

  /* Construct graph */
  color = gegl_node_new_child (NULL,
                               "operation", "gegl:color",
                               NULL);

  /* Run tests */
  for (i = 0; i < RUNS; i++)
    {
      gegl_node_set (color,
                     "value", colors[i],
                     NULL);
      memset (result_buffer, 0, sizeof (result_buffer));
      gegl_node_blit (color,
                      1.0,
                      &rois[i],
                      babl_format ("RGB u8"),
                      result_buffer,
                      GEGL_AUTO_ROWSTRIDE,
                      GEGL_BLIT_DEFAULT);
      if (!(result_buffer[RED]   == expected_results[i][RED]   &&
            result_buffer[GREEN] == expected_results[i][GREEN] &&
            result_buffer[BLUE]  == expected_results[i][BLUE]))
        {
          result = FAILURE;
          g_printerr ("Processing #%d of gegl:color failed", i + 1);
          break;
        }
    }

  /* Cleanup */
  g_object_unref (color);
  for (i = 0; i < RUNS; i++)
    g_object_unref (colors[i]);
  gegl_exit ();

  return result;
}
