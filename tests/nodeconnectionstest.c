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
test_node_connections(Test *test)
{
  /*
       -
       B
       +
       |
       -
       A

  */

  {
    GList *sources;
    GList *sinks;
    GeglConnection *connection;

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(B, "input0");

    gegl_node_connect(B, "input0", A, "output0");

    ct_test(test, 1 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(B));
    ct_test(test, 1 == gegl_node_get_num_sources(B));
    ct_test(test, 1 == gegl_pad_get_num_connections(output0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input0));
    ct_test(test, output0 == gegl_pad_get_connected_to(input0));

    sinks = gegl_node_get_sinks(A);
    connection = g_list_nth_data(sinks, 0);
    ct_test(test, A == gegl_connection_get_source_node(connection));
    ct_test(test, output0 == gegl_connection_get_source_pad(connection));

    sources = gegl_node_get_sources(B);
    connection = g_list_nth_data(sources, 0);
    ct_test(test, B == gegl_connection_get_sink_node(connection));
    ct_test(test, input0 == gegl_connection_get_sink_pad(connection));

    gegl_node_disconnect(B, "input0", A, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(B));
    ct_test(test, 0 == gegl_node_get_num_sources(B));
    ct_test(test, 0 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));

    g_object_unref(A);
    g_object_unref(B);
  }

  /*
        -
        C
       + +
       \
        -
        A

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation21", NULL);
    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(C, "input0");
    GeglPad *input1 = gegl_node_get_pad(C, "input1");

    gegl_node_connect(C, "input0", A, "output0");

    ct_test(test, 1 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 1 == gegl_node_get_num_sources(C));
    ct_test(test, 1 == gegl_pad_get_num_connections(output0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input1));
    ct_test(test, output0 == gegl_pad_get_connected_to(input0));

    gegl_node_disconnect(C, "input0", A, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 0 == gegl_node_get_num_sources(C));
    ct_test(test, 0 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input1));

    g_object_unref(A);
    g_object_unref(C);
  }

  /*
       -
       C
      + +
        /
       -
       A

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation21", NULL);
    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(C, "input0");
    GeglPad *input1 = gegl_node_get_pad(C, "input1");

    gegl_node_connect(C, "input1", A, "output0");

    ct_test(test, 1 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 1 == gegl_node_get_num_sources(C));
    ct_test(test, 1 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input1));
    ct_test(test, output0 == gegl_pad_get_connected_to(input1));

    gegl_node_disconnect(C, "input1", A, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 0 == gegl_node_get_num_sources(C));
    ct_test(test, 0 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input1));

    g_object_unref(A);
    g_object_unref(C);
  }

  /*
       -
       C
      + +
      \ /
       -
       A

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation21", NULL);
    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(C, "input0");
    GeglPad *input1 = gegl_node_get_pad(C, "input1");

    gegl_node_connect(C, "input1", A, "output0");
    gegl_node_connect(C, "input0", A, "output0");

    ct_test(test, 2 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 2 == gegl_node_get_num_sources(C));
    ct_test(test, 2 == gegl_pad_get_num_connections(output0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input1));
    ct_test(test, output0 == gegl_pad_get_connected_to(input0));
    ct_test(test, output0 == gegl_pad_get_connected_to(input1));

    gegl_node_disconnect(C, "input1", A, "output0");
    gegl_node_disconnect(C, "input0", A, "output0");

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(C));
    ct_test(test, 0 == gegl_node_get_num_sources(C));
    ct_test(test, 0 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input1));

    g_object_unref(A);
    g_object_unref(C);
  }

  /*
       -
       C
      + +
      | |
      - -
      A B

  */

  {
    GList *sources;
    GeglConnection *connection;

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation01", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation21", NULL);
    GeglPad *Aoutput0 = gegl_node_get_pad(A, "output0");
    GeglPad *Boutput0 = gegl_node_get_pad(B, "output0");
    GeglPad *input0 = gegl_node_get_pad(C, "input0");
    GeglPad *input1 = gegl_node_get_pad(C, "input1");

    gegl_node_connect(C, "input0", A, "output0");
    gegl_node_connect(C, "input1", B, "output0");

    ct_test(test, 1 == gegl_node_get_num_sinks(A));
    ct_test(test, 1 == gegl_node_get_num_sinks(B));
    ct_test(test, 2 == gegl_node_get_num_sources(C));

    ct_test(test, 1 == gegl_pad_get_num_connections(Aoutput0));
    ct_test(test, 1 == gegl_pad_get_num_connections(Boutput0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input0));
    ct_test(test, 1 == gegl_pad_get_num_connections(input1));
    ct_test(test, Aoutput0 == gegl_pad_get_connected_to(input0));
    ct_test(test, Boutput0 == gegl_pad_get_connected_to(input1));

    sources = gegl_node_get_sources(C);

    connection = g_list_nth_data(sources, 0);
    ct_test(test, C == gegl_connection_get_sink_node(connection));
    ct_test(test, A == gegl_connection_get_source_node(connection));
    ct_test(test, Aoutput0 == gegl_connection_get_source_pad(connection));
    ct_test(test, input0 == gegl_connection_get_sink_pad(connection));

    connection = g_list_nth_data(sources, 1);
    ct_test(test, C == gegl_connection_get_sink_node(connection));
    ct_test(test, B == gegl_connection_get_source_node(connection));
    ct_test(test, Boutput0 == gegl_connection_get_source_pad(connection));
    ct_test(test, input1 == gegl_connection_get_sink_pad(connection));

    gegl_node_disconnect_sources(C);

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sinks(B));
    ct_test(test, 0 == gegl_node_get_num_sources(C));

    ct_test(test, 0 == gegl_pad_get_num_connections(Aoutput0));
    ct_test(test, 0 == gegl_pad_get_num_connections(Boutput0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input0));
    ct_test(test, 0 == gegl_pad_get_num_connections(input1));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  /*
     -   -
     B   C
     +   +
      \ /
       -
       A
  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "operation", "GeglMockOperation11", NULL);

    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *Binput0 = gegl_node_get_pad(B, "input0");
    GeglPad *Cinput0 = gegl_node_get_pad(C, "input0");

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", A, "output0");

    ct_test(test, 2 == gegl_node_get_num_sinks(A));
    ct_test(test, 1 == gegl_node_get_num_sources(B));
    ct_test(test, 1 == gegl_node_get_num_sources(C));
    ct_test(test, 2 == gegl_pad_get_num_connections(output0));
    ct_test(test, 1 == gegl_pad_get_num_connections(Binput0));
    ct_test(test, 1 == gegl_pad_get_num_connections(Cinput0));
    ct_test(test, output0 == gegl_pad_get_connected_to(Binput0));
    ct_test(test, output0 == gegl_pad_get_connected_to(Cinput0));

    gegl_node_disconnect_sinks(A);

    ct_test(test, 0 == gegl_node_get_num_sinks(A));
    ct_test(test, 0 == gegl_node_get_num_sources(B));
    ct_test(test, 0 == gegl_node_get_num_sources(C));
    ct_test(test, 0 == gegl_pad_get_num_connections(output0));
    ct_test(test, 0 == gegl_pad_get_num_connections(Binput0));
    ct_test(test, 0 == gegl_pad_get_num_connections(Cinput0));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }
}

static void
node_connections_test_setup(Test *test)
{
}

static void
node_connections_test_teardown(Test *test)
{
}

Test *
create_node_connections_test()
{
  Test* t = ct_create("GeglNodeConnectionsTest");

  g_assert(ct_addSetUp(t, node_connections_test_setup));
  g_assert(ct_addTearDown(t, node_connections_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_node_connections));
#endif

  return t;
}
