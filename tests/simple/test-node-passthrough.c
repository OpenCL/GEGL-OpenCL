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
 * Copyright (C) 2017 Red Hat, Inc.
 */

#include "config.h"
#include <stdio.h>

#include "gegl.h"

static void
sub_graph_crop_passthrough_invalidated (GeglNode *node, GeglRectangle *rect, gpointer user_data)
{
  GeglRectangle bbox;
  gboolean *result = (gboolean *) user_data;

  bbox = gegl_node_get_bounding_box (node);

  if (bbox.x != 0 || bbox.y != 0 || bbox.width != 100 || bbox.height != 100)
    *result = FALSE;
}

static gboolean
test_sub_graph_crop_passthrough (void)
{
  GeglNode *color;
  GeglNode *crop;
  GeglNode *crop_color;
  GeglNode *graph;
  GeglNode *input;
  GeglNode *output;
  GeglNode *sub_graph;
  gboolean result = TRUE;
  gulong invalidated_id;

  graph = gegl_node_new ();

  color = gegl_node_new_child (graph,
                               "operation", "gegl:color",
                               NULL);

  crop_color = gegl_node_new_child (graph,
                              "operation", "gegl:crop",
                              "x", 0.0,
                              "y", 0.0,
                              "width", 100.0,
                              "height", 100.0,
                              NULL);

  sub_graph = gegl_node_new ();
  gegl_node_add_child (graph, sub_graph);
  input = gegl_node_get_input_proxy (sub_graph, "input");
  output = gegl_node_get_output_proxy (sub_graph, "output");

  crop = gegl_node_new_child (sub_graph,
                              "operation", "gegl:crop",
                              "x", 10.0,
                              "y", 10.0,
                              "width", 10.0,
                              "height", 10.0,
                              NULL);

  gegl_node_link_many (input, crop, output, NULL);
  gegl_node_link_many (color, crop_color, sub_graph, NULL);
  gegl_node_process (sub_graph);

  invalidated_id = g_signal_connect (sub_graph,
                                     "invalidated",
                                     G_CALLBACK (sub_graph_crop_passthrough_invalidated),
                                     &result);

  gegl_node_set_passthrough (crop, TRUE);

  g_signal_handler_disconnect (sub_graph, invalidated_id);

  g_object_unref (graph);
  g_object_unref (sub_graph);

  return result;
}

#define RUN_TEST(test_name) \
{ \
  if (test_name()) \
    { \
      printf ("" #test_name " ... PASS\n"); \
      tests_passed++; \
    } \
  else \
    { \
      printf ("" #test_name " ... FAIL\n"); \
      tests_failed++; \
    } \
  tests_run++; \
}

int
main (int argc, char **argv)
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gegl_init (0, NULL);
  g_object_set(G_OBJECT(gegl_config()),
               "swap", "RAM",
               "use-opencl", FALSE,
               NULL);

  RUN_TEST (test_sub_graph_crop_passthrough)

  gegl_exit ();

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
