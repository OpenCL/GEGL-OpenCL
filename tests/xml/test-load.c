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

/* Test loading/de-serialization of graphs from XML */

#include <glib.h>

#include <gegl.h>

#include "common.c"

/* Loading an empty graph should result a valid GeglNode with no children.
 *
 * Kind-of a sanity test, should run before other tests. */
static void
test_load_empty_graph (void)
{
    const gchar * const xml = "<?xml version='1.0' encoding='UTF-8'?>\n<gegl>\n</gegl>\n";
    GeglNode *graph;
	GSList *children;

    graph = gegl_node_new_from_xml(xml, "");
    g_assert(graph);

    children = gegl_node_get_children(graph);
    g_assert_cmpuint(g_slist_length(children), ==, 0);

	g_slist_free(children);
    g_object_unref(graph);
}


/* Loading a graph with X child nodes should result in
 * a GeglNode with X children, where the bottom-most node
 * is child 0.
 *
 * Note: Properties are not tested. */
static void
test_load_multiple_nodes (void)
{
    const gchar * const xml = \
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

    GeglNode *graph, *node;
	GSList *children;
    gchar *op_name;

    graph = gegl_node_new_from_xml(xml, "");
    g_assert(graph);

    children = gegl_node_get_children(graph);
    g_assert_cmpuint(g_slist_length(children), ==, 2);

    node = GEGL_NODE(g_slist_nth_data(children, 0));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, "gegl:crop");
    g_free(op_name);

    node = GEGL_NODE(g_slist_nth_data(children, 1));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, "gegl:invert");
    g_free(op_name);

	g_slist_free(children);
    g_object_unref(graph);
}


/* Test that loading a subgraph works */
static void
test_load_subgraph (void)
{
    const gchar * const xml = \
"<?xml version='1.0' encoding='UTF-8'?>\n\
<gegl>\n\
    <node>\n\
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
    </node>\n\
    <node operation='gegl:crop'>\n\
    </node>\n\
</gegl>\n";

    GeglNode *graph, *node;
	GSList *toplevel_children, *subgraph_children;
    gchar *op_name;

    graph = gegl_node_new_from_xml(xml, "");
    g_assert(graph);

    toplevel_children = gegl_node_get_children(graph);
    g_assert_cmpuint(g_slist_length(toplevel_children), ==, 2);

    node = GEGL_NODE(g_slist_nth_data(toplevel_children, 0));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, "gegl:crop");
    g_free(op_name);


    node = GEGL_NODE(g_slist_nth_data(toplevel_children, 1));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, ""); // Meta-operation
    g_free(op_name);

    /* Subgraph */
    subgraph_children = gegl_node_get_children(node);
    g_assert_cmpuint(g_slist_length(toplevel_children), ==, 2);

    node = GEGL_NODE(g_slist_nth_data(subgraph_children, 0));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, "gegl:crop");
    g_free(op_name);

    node = GEGL_NODE(g_slist_nth_data(subgraph_children, 1));
    gegl_node_get(node, "operation", &op_name, NULL);
    g_assert_cmpstr(op_name, ==, "gegl:invert");
    g_free(op_name);

	g_slist_free(subgraph_children);
	g_slist_free(toplevel_children);
    g_object_unref(graph);
}


int
main (int argc, char *argv[])
{
    int result = -1;

    gegl_init(&argc, &argv);

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/xml/load/empty_graph", test_load_empty_graph);
    g_test_add_func("/xml/load/multiple_nodes", test_load_multiple_nodes);

    /* Expected failure: not implemented
    g_test_add_func("/xml/load/subgraph", test_load_subgraph);
    */

    result = g_test_run();
    gegl_exit();
    return result;
}
