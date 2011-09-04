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
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
 */

/* Test saving/serialization of graphs to XML
 */

#include <glib.h>

#include <gegl.h>

#include "common.c"

/* Saving an empty graph should result in a valid UTF-8 encoded XML document,
 * with the same basic structure as if the graph was non-empty.
 *
 * Kind-of a sanity test, should run before other tests. */
static void
test_save_empty_graph (void)
{
    const gchar * const expected_result = "<?xml version='1.0' encoding='UTF-8'?>\n<gegl>\n</gegl>\n";
    GeglNode *graph;
    gchar *xml;

    graph = gegl_node_new();
    xml = gegl_node_to_xml(graph, "");

    assert_equivalent_xml(xml, expected_result);

    g_object_unref(graph);
    g_free(xml);
}

/* gegl:nop nodes should be discarded when saving the graph */
static void
test_save_nop_nodes (void)
{
    const gchar * const expected_result = "<?xml version='1.0' encoding='UTF-8'?>\n<gegl>\n</gegl>\n";
    GeglNode *graph, *op1, *op2;
    gchar *xml;

    graph = gegl_node_new();
    op1 = gegl_node_new_child(graph, "operation", "gegl:nop", NULL);
    op2 = gegl_node_new_child(graph, "operation", "gegl:nop", NULL);
    gegl_node_link_many(op1, op2, NULL);

    xml = gegl_node_to_xml(op2, "");

    assert_equivalent_xml(xml, expected_result);

    g_object_unref(graph);
    g_free(xml);
}


/* Saving a graph with multiple nodes
 *
 * Note: Relies on the number and names of properties of the used operations to not change */
static void
test_save_multiple_nodes (void)
{
    const gchar * const expected_result = \
"<?xml version='1.0' encoding='UTF-8'?>\n\
<gegl>\n\
  <node operation='gegl:invert'>\n\
  </node>\n\
  <node operation='gegl:crop'>\n\
      <params>\n\
        <param name='x'>0</param>\n\
        <param name='y'>0</param>\n\
        <param name='width'>0</param>\n\
        <param name='height'>0</param>\n\
      </params>\n\
  </node>\n\
</gegl>\n";

    GeglNode *graph, *op1, *op2;
    gchar *xml;

    graph = gegl_node_new();
    op1 = gegl_node_new_child(graph, "operation", "gegl:crop",
                              "x", 0.0, "y", 0.0,
                              "width", 0.0, "height", 0.0,
                              NULL);
    op2 = gegl_node_new_child(graph, "operation", "gegl:invert", NULL);
    gegl_node_link_many(op1, op2, NULL);

    xml = gegl_node_to_xml(op2, "");

    assert_equivalent_xml(xml, expected_result);

    g_object_unref(graph);
    g_free(xml);
}


/* Test that saving a subgraph works */
static void
test_save_toplevel_graph (void)
{
    const gchar * const expected_result = \
"<?xml version='1.0' encoding='UTF-8'?>\n\
<gegl>\n\
  <node operation='gegl:invert'>\n\
  </node>\n\
  <node operation='gegl:crop'>\n\
      <params>\n\
        <param name='x'>0</param>\n\
        <param name='y'>0</param>\n\
        <param name='width'>0</param>\n\
        <param name='height'>0</param>\n\
      </params>\n\
  </node>\n\
</gegl>\n";

    GeglNode *graph, *op1, *op2;
    gchar *xml;

    graph = gegl_node_new();
    op1 = gegl_node_new_child(graph, "operation", "gegl:crop",
                              "x", 0.0, "y", 0.0,
                              "width", 0.0, "height", 0.0,
                              NULL);
    op2 = gegl_node_new_child(graph, "operation", "gegl:invert", NULL);
    gegl_node_link_many(op1, op2, NULL);

    xml = gegl_node_to_xml(graph, "");

    assert_equivalent_xml(xml, expected_result);

    g_object_unref(graph);
    g_free(xml);
}


int
main (int argc, char *argv[])
{
    int result = -1;

    gegl_init(&argc, &argv);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/xml/save/empty_graph", test_save_empty_graph);
    g_test_add_func("/xml/save/only_nop_nodes", test_save_nop_nodes);
    g_test_add_func("/xml/save/multiple_nodes", test_save_multiple_nodes);

    /* Expected failure
    g_test_add_func("/xml/save/toplevel_graph", test_save_toplevel_graph);
    */

    result = g_test_run();
    gegl_exit();
    return result;
}
