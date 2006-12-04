#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-operation-0-1.h"
#include "gegl-mock-operation-1-1.h"
#include "gegl-mock-operation-2-1.h"
#include "gegl-mock-operation-2-2.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_graph_g_object_new(Test *test)
{
  {
    GeglNode * graph = g_object_new (GEGL_TYPE_GRAPH, NULL);

    ct_test(test, GEGL_IS_GRAPH(graph));
    ct_test(test, g_type_parent(GEGL_TYPE_GRAPH) == GEGL_TYPE_NODE);
    ct_test(test, !strcmp("GeglGraph", g_type_name(GEGL_TYPE_GRAPH)));

    g_object_unref(graph);
  }
}

static void
test_graph(Test *test)
{
  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglGraph *graph = g_object_new (GEGL_TYPE_GRAPH, NULL);

    gegl_graph_add_child (graph, A);
    ct_test(test, 1 == gegl_graph_get_num_children(graph));

    gegl_graph_remove_child (graph, A);
    ct_test(test, 0 == gegl_graph_get_num_children(graph));

    g_object_unref(A);
    g_object_unref(graph);
  }
}

static void
test_graph_properties(Test *test)
{
  /*
      B
      +
      |
      -
     ---
    |   |  <----graph with A as child
    | - |
    | A |
    | + |
    |   |
     ---
      +
  */
  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglGraph *graph = g_object_new (GEGL_TYPE_GRAPH, NULL);

    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(A, "input0");

    gegl_graph_add_child(graph, A);
    gegl_node_add_pad(GEGL_NODE(graph), output0);
    gegl_node_add_pad(GEGL_NODE(graph), input0);

    ct_test(test, 1 == gegl_node_get_num_output_pads(GEGL_NODE(graph)));
    ct_test(test, 1 == gegl_node_get_num_input_pads(GEGL_NODE(graph)));

    gegl_node_connect_from(B, "input0", GEGL_NODE(graph), "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 1 == gegl_node_get_num_sinks(GEGL_NODE(graph)));
    ct_test(test, 1 == gegl_node_get_num_sources(B));

    ct_test(test, output0 == gegl_node_get_pad(GEGL_NODE(graph), "output0"));
    ct_test(test, GEGL_NODE(A) == gegl_pad_get_node(output0));

    gegl_node_disconnect(B, "input0", GEGL_NODE(graph), "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(GEGL_NODE(graph)));
    ct_test(test, 0 == gegl_node_get_num_sources(B));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(graph);
  }

  /*
      B
      +
      |
      -
     ---
    |   |  <----graph with A as child
    | - |
    | A |
    | + |
    |   |
     ---
      +
      |
      -
      C

  */
  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation01", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_GRAPH, NULL);

    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(A, "input0");

    gegl_graph_add_child(GEGL_GRAPH(D), A);
    gegl_node_add_pad(D, output0);
    gegl_node_add_pad(D, input0);

    ct_test(test, 1 == gegl_node_get_num_output_pads(D));
    ct_test(test, 1 == gegl_node_get_num_input_pads(D));

    gegl_node_connect_from(B, "input0", D, "output0");
    gegl_node_connect_from(D, "input0", C, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 1 == gegl_node_get_num_sources(B));
    ct_test(test, 1 == gegl_node_get_num_sinks(C));
    ct_test(test, 1 == gegl_node_get_num_sinks(D));
    ct_test(test, 1 == gegl_node_get_num_sources(D));

    ct_test(test, output0 == gegl_node_get_pad(D, "output0"));
    ct_test(test, A == gegl_pad_get_node (output0));

    ct_test(test, input0 == gegl_node_get_pad(D, "input0"));
    ct_test(test, A == gegl_pad_get_node (input0));

    gegl_node_disconnect(B, "input0", D, "output0");
    gegl_node_disconnect(D, "input0", C, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sources(B));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 0 == gegl_node_get_num_sinks(D));
    ct_test(test, 0 == gegl_node_get_num_sources(D));

    ct_test(test, 1 == gegl_node_get_num_output_pads(D));
    ct_test(test, 1 == gegl_node_get_num_input_pads(D));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }
}

static void
test_graph_property_visitors(Test *test)
{
  /*
      B
      +
      |
      -
     ---
    |   |  <----graph with A as child
    | - |
    | A |
    | + |
    |   |
     ---
      +
      |
      -
      C

  */
  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation01", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_GRAPH, NULL);

    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(A, "input0");

    gegl_graph_add_child(GEGL_GRAPH(D), A);
    gegl_node_add_pad(D, output0);
    gegl_node_add_pad(D, input0);

    ct_test(test, 1 == gegl_node_get_num_output_pads(D));
    ct_test(test, 1 == gegl_node_get_num_input_pads(D));

    gegl_node_connect_from(B, "input0", D, "output0");
    gegl_node_connect_from(D, "input0", C, "output0");

    gegl_node_disconnect(B, "input0", D, "output0");
    gegl_node_disconnect(D, "input0", C, "output0");

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }
}

static void
setup(Test *test)
{
}

static void
teardown(Test *test)
{
}

Test *
create_graph_test()
{
  Test* t = ct_create("GeglGraphTest");

  g_assert(ct_addSetUp(t, setup));
  g_assert(ct_addTearDown(t, teardown));

#if 1
  g_assert(ct_addTestFun(t, test_graph_g_object_new));
  g_assert(ct_addTestFun(t, test_graph));
  g_assert(ct_addTestFun(t, test_graph_properties));
  g_assert(ct_addTestFun(t, test_graph_property_visitors));
#endif

  return t;
}
