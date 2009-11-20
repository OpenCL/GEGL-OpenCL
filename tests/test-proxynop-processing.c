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

#include "config.h"
#include <string.h>

#include "gegl.h"

#define RED      0
#define GREEN    1
#define BLUE     2

#define SUCCESS  0
#define FAILURE -1

int main(int argc, char *argv[])
{
  int           result             = SUCCESS;
  GeglNode     *graph              = NULL;
  GeglNode     *color_in_graph     = NULL;
  GeglNode     *crop_in_graph      = NULL;
  GeglNode     *graph_output_proxy = NULL;
  GeglNode     *crop_outside_graph = NULL;
  GeglRectangle roi                = { 0, 0, 1, 1 };
  guchar        result_buffer[3]   = { 0, 0, 0 };
  GeglColor    *color1             = NULL;
  GeglColor    *color2             = NULL;

  /* Init */
  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  color1 = gegl_color_new ("rgb(1.0, 0.0, 1.0)");
  color2 = gegl_color_new ("rgb(0.0, 0.0, 1.0)");

  /* Construct graph */
  graph          = gegl_node_new ();
  color_in_graph = gegl_node_new_child (graph,
                                        "operation", "gegl:color",
                                        "value",     color1,
                                        NULL);
  crop_in_graph  = gegl_node_new_child (graph,
                                        "operation", "gegl:crop",
                                        "x",         0.0,
                                        "y",         0.0,
                                        "width",     1.0,
                                        "height",    1.0,
                                        NULL);
  graph_output_proxy = gegl_node_get_output_proxy (graph,
                                                   "output");

  gegl_node_link_many (color_in_graph,
                       crop_in_graph,
                       graph_output_proxy,
                       NULL);

  /* Make sure the implicit connection to the graph output proxy works
   * by connecting an additional crop directly to the graph node
   */
  crop_outside_graph  = gegl_node_new_child (graph,
                                             "operation", "gegl:crop",
                                             "x",         0.0,
                                             "y",         0.0,
                                             "width",     1.0,
                                             "height",    1.0,
                                             NULL);
  gegl_node_connect_to (graph,              "output",
                        crop_outside_graph, "input");

  /* Process the graph and make sure we get the expected result */
  memset (result_buffer, 0, sizeof (result_buffer));
  gegl_node_blit (crop_outside_graph,
                  1.0,
                  &roi,
                  babl_format ("RGB u8"),
                  result_buffer,
                  GEGL_AUTO_ROWSTRIDE,
                  GEGL_BLIT_DEFAULT);
  if (!(result_buffer[RED]   == 255 &&
        result_buffer[GREEN] == 0   &&
        result_buffer[BLUE]  == 255))
    {
      result = FAILURE;
      g_printerr ("Initial processing failed, you messed up GEGL pretty badly :(");
      goto abort;
    }

  /* Process the graph again with a different color and make sure we
   * get the expected result
   */
  gegl_node_set (color_in_graph,
                 "value", color2,
                 NULL);
  memset (result_buffer, 0, sizeof (result_buffer));
  gegl_node_blit (crop_outside_graph,
                  1.0,
                  &roi,
                  babl_format ("RGB u8"),
                  result_buffer,
                  GEGL_AUTO_ROWSTRIDE,
                  GEGL_BLIT_DEFAULT);
  if (!(result_buffer[RED]   == 0   &&
        result_buffer[GREEN] == 0   &&
        result_buffer[BLUE]  == 255))
    {
      result = FAILURE;
      g_printerr ("Second processing failed, i.e. changing color didn't work properly");
      goto abort;
    }

  /* Process the graph again but without changing anything since this
   * puts some stress on the caching mechanisms
   */
  memset (result_buffer, 0, sizeof (result_buffer));
  gegl_node_blit (crop_outside_graph,
                  1.0,
                  &roi,
                  babl_format ("RGB u8"),
                  result_buffer,
                  GEGL_AUTO_ROWSTRIDE,
                  GEGL_BLIT_DEFAULT);
  if (!(result_buffer[RED]   == 0   &&
        result_buffer[GREEN] == 0   &&
        result_buffer[BLUE]  == 255))
    {
      result = FAILURE;
      g_printerr ("Third processing failed, looks like you messed up caching");
      goto abort;
    }

abort:
  /* Cleanup */
  g_object_unref (graph);
  g_object_unref (color1);
  g_object_unref (color2);
  gegl_exit ();

  return result;
}
