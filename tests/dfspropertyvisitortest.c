#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-property-visitor.h"
#include "gegl-mock-operation-0-1.h"
#include "gegl-mock-operation-1-1.h"
#include "gegl-mock-operation-1-2.h"
#include "gegl-mock-operation-2-1.h"
#include "gegl-mock-operation-2-2.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>


static gboolean
do_visitor_and_check_visit_order(gchar **visit_order,
                                 gint length,
                                 gchar *pad_name,
                                 GeglNode *node)
{
  gint i;
  GList *visits_list = NULL;
  GeglPad     *pad = gegl_node_get_pad (node, pad_name);
  GeglVisitor *  visitor = g_object_new(GEGL_TYPE_MOCK_PROPERTY_VISITOR, NULL);
  gegl_visitor_dfs_traverse(visitor, GEGL_VISITABLE(pad));

  visits_list = gegl_visitor_get_visits_list(visitor);

  if(length != (gint)g_list_length(visits_list))
      return FALSE;

  for(i = 0; i < length; i++)
    {
      GeglPad *pad = (GeglPad*)g_list_nth_data(visits_list, i);
      GeglOperation *operation = gegl_pad_get_operation(pad);
      const gchar *node_name = gegl_object_get_name(GEGL_OBJECT(operation));
      gchar *pad_name =
        g_strconcat(node_name, ".", gegl_pad_get_name(pad), NULL);

      if(0 != strcmp(pad_name, visit_order[i]))
        {
          g_free(pad_name);
          g_object_unref(visitor);
          return FALSE;
        }

      g_free(pad_name);
    }

  g_object_unref(visitor);
  return TRUE;
}

static void
test_dfs_property_visitor(Test *test)
{
  /*
       -
       C
      + +
      | |
      - -
      A B
  */

  {
    gchar * visit_order[] = { "A.output0",
                              "C.input0",
                              "B.output0",
                              "C.input1",
                              "C.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "C", NULL);

    gegl_node_connect(C, "input0", A, "output0");
    gegl_node_connect(C, "input1", B, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 5, "output0", C));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  /*
     -
     C
     +
     |
     -
     B
     +
     |
     -
     A
  */
  {
    gchar * visit_order[] = { "A.output0",
                              "B.input0",
                              "B.output0",
                              "C.input0",
                              "C.output0" };

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "C", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");

    ct_test(test, do_visitor_and_check_visit_order(visit_order, 5, "output0", C));

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }


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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "B", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "C", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_2, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "C", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_2, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "B", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "C", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "C", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "D", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "C", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_MOCK_OPERATION_2_1, "name", "D", NULL);

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

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "A", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1, "name", "B", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, "name", "C", NULL);
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
}

static void
dfs_property_visitor_test_setup(Test *test)
{
}

static void
dfs_property_visitor_test_teardown(Test *test)
{
}

Test *
create_dfs_property_visitor_test()
{
  Test* t = ct_create("GeglDfsPropertyVisitorTest");

  g_assert(ct_addSetUp(t, dfs_property_visitor_test_setup));
  g_assert(ct_addTearDown(t, dfs_property_visitor_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_dfs_property_visitor));
#endif

  return t;
}
