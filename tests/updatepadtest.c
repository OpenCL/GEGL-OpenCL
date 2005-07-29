#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-operation-0-1.h"
#include "gegl-mock-operation-1-1.h"
#include "gegl-mock-operation-1-2.h"
#include "gegl-mock-operation-2-1.h"
#include "gegl-mock-operation-2-2.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>


static void
test_update_pad (Test *test)
{
  /*
     -
     A
  */

  {
    gint output0;
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);

    gegl_node_apply (A, "output0");
    gegl_node_get (A, "output0", &output0, NULL);
    ct_test(test, 1 == output0);

    g_object_unref(A);
  }

  /*
     -
     B
     +
         <--- Test passing the property from output to input
              (like visit_property in EvalComputeVisitor)
     -
     A
  */
  {
    GValue value = {0};
    gint input0;

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);

    g_value_init(&value, G_TYPE_INT);

    gegl_node_apply(A, "output0");

    gegl_node_get_property(G_OBJECT(A), "output0", &value);
    gegl_node_set_property(G_OBJECT(B), "input0", (const GValue*)&value);

    gegl_node_get (B, "input0", &input0, NULL);
    ct_test(test, 1 == input0);

    g_object_unref(A);
    g_object_unref(B);
  }


  /*
     -      2( = 2*1)
     B
     +      1
     |
     -      1
     A
  */
  {
    gint output0;

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_apply(B, "output0");

    gegl_node_get (B, "output0", &output0, NULL);
    ct_test(test, 2 == output0);

    g_object_unref(A);
    g_object_unref(B);
  }

  /*
     -   4
     C
     +   2
     |
     -   2
     B
     +   1
     |
     -   1
     A
  */
  {
    gint output0;
    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");

    gegl_node_apply(C, "output0");

    gegl_node_get (C, "output0", &output0, NULL);
    ct_test(test, 4 == output0);

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

#if 0

  /*
       -
       B
      + +
      \ /
       -
       A
  */

  {
    gchar * visit_order[] = { "A.output0",
                              "B.input0",
                              "B.input1",
                              "B.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(B, "input1", A, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 4, "output0", B));

    g_object_unref(A);
    g_object_unref(B);
  }

  /*
       -
       C
      + +
        /
       -
       B
      + +
      \
       -
       A
  */

  {
    gchar * visit_order[] = { "C.input0",
                              "A.output0",
                              "B.input0",
                              "B.input1",
                              "B.output0",
                              "C.input1",
                              "C.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input1", B, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 7, "output0", C));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  /*
       -
       C
      + +
      | |
      - -
       B
      + +
      \ /
       -
       A
  */

  {
    gchar * visit_order[] = { "A.output0",
                              "B.input0",
                              "B.input1",
                              "B.output0",
                              "C.input0",
                              "B.output1",
                              "C.input1",
                              "C.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation22", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(B, "input1", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");
    gegl_node_connect(C, "input1", B, "output1");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 8, "output0", C));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  /*
       -
       B
      + +
      | |
      - -
       A
       +
  */

  {
    gchar * visit_order[] = { "A.input0",
                              "A.output0",
                              "B.input0",
                              "A.output1",
                              "B.input1",
                              "B.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation12", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(B, "input1", A, "output1");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 6, "output0", B));

    g_object_unref(A);
    g_object_unref(B);
  }

  /*
      -
      C
     + +
     | |
     | -
     | B
     | +
     \ /
      -
      A
  */

  {
    gchar * visit_order[] = { "A.output0",
                              "C.input0",
                              "B.input0",
                              "B.output0",
                              "C.input1",
                              "C.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input1", B, "output0");
    gegl_node_connect(C, "input0", A, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 6, "output0", C));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  /*
      -
      D
     + +
     | |
     | -
     | C
     | +
     \ /
      -
      B
      +
      |
      -
      A
      +

  */

  {
    gchar * visit_order[] = { "A.input0",
                              "A.output0",
                              "B.input0",
                              "B.output0",
                              "D.input0",
                              "C.input0",
                              "C.output0",
                              "D.input1",
                              "D.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_NODE, "name", "D", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");
    gegl_node_connect(D, "input0", B, "output0");
    gegl_node_connect(D, "input1", C, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 9, "output0", D));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }

  /*
      -
      D
     + +
     | |
     | -
     | C
     | +
     | |
     | -
     | B
     | +
     \ /
      -
      A
  */

  {
    gchar * visit_order[] = { "A.output0",
                              "D.input0",
                              "B.input0",
                              "B.output0",
                              "C.input0",
                              "C.output0",
                              "D.input1",
                              "D.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_NODE, "name", "D", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");
    gegl_node_connect(D, "input1", C, "output0");
    gegl_node_connect(D, "input0", A, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 8, "output0", D));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
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
    gchar * visit_order[] = { "C.output0",
                              "A.input0",
                              "A.output0",
                              "B.input0",
                              "B.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation01", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_GRAPH, "name", "D", NULL);

    GeglPad *output0 = gegl_node_get_pad(A, "output0");
    GeglPad *input0 = gegl_node_get_pad(A, "input0");

    gegl_graph_add_child(GEGL_GRAPH(D), A);
    gegl_node_add_pad(D, output0);
    gegl_node_add_pad(D, input0);

    gegl_node_connect(B, "input0", D, "output0");
    gegl_node_connect(D, "input0", C, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 5, "output0", B));

    gegl_node_disconnect(B, "input0", D, "output0");
    gegl_node_disconnect(D, "input0", C, "output0");

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }
#endif
}

static void
test_setup (Test *test)
{
}

static void
test_teardown (Test *test)
{
}

Test *
create_update_pad_test()
{
  Test* t = ct_create ("GeglUpdatePropertyTest");

  g_assert (ct_addSetUp(t, test_setup));
  g_assert (ct_addTearDown(t, test_teardown));

#if 1
  g_assert (ct_addTestFun(t, test_update_pad));
#endif

  return t;
}
