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
                     gchar *prop_name,
                     GeglNode *node)
{
  gint i;
  GList *visits_list = NULL;
  GeglProperty *property = gegl_node_get_property(node, prop_name);
  GeglVisitor *  visitor = g_object_new(GEGL_TYPE_MOCK_PROPERTY_VISITOR, NULL);

  gegl_visitor_bfs_traverse(visitor, GEGL_VISITABLE(property));

  visits_list = gegl_visitor_get_visits_list(visitor);

  if(length != (gint)g_list_length(visits_list))
      return FALSE;

  for(i = 0; i < length; i++)
    {
      GeglProperty *property = g_list_nth_data(visits_list, i);
      GeglOperation *operation = gegl_property_get_operation(property);
      const gchar *node_name = gegl_object_get_name(GEGL_OBJECT(operation));
      gchar *property_name = g_strconcat(node_name,
                                         ".",
                                         gegl_property_get_name(property),
                                         NULL);

      if(0 != strcmp(property_name, visit_order[i]))
        {
          g_free(property_name);
          g_object_unref(visitor);
          return FALSE;
        }

      g_free(property_name);
    }

  g_object_unref(visitor);
  return TRUE;
}

static void
test_bfs_property_visitor(Test *test)
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
    gchar * visit_order[] = { "C.output0",
                              "C.input0",
                              "C.input1",
                              "A.output0",
                              "B.output0"};

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
    gchar * visit_order[] = { "C.output0",
                              "C.input0",
                              "B.output0",
                              "B.input0",
                              "A.output0" };

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
    gchar * visit_order[] = { "B.output0",
                              "B.input0",
                              "B.input1",
                              "A.output0" };

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
    gchar * visit_order[] = { "C.output0",
                              "C.input0",
                              "C.input1",
                              "B.output0",
                              "B.input0",
                              "B.input1",
                              "A.output0" };

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
    gchar * visit_order[] = { "C.output0",
                              "C.input0",
                              "C.input1",
                              "B.output0",
                              "B.output1",
                              "B.input0",
                              "B.input1",
                              "A.output0" };

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
    gchar * visit_order[] = { "B.output0",
                              "B.input0",
                              "B.input1",
                              "A.output0",
                              "A.output1",
                              "A.input0" };

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
    gchar * visit_order[] = { "C.output0",
                              "C.input0",
                              "C.input1",
                              "B.output0",
                              "B.input0",
                              "A.output0" };

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
    gchar * visit_order[] = { "D.output0",
                              "D.input0",
                              "D.input1",
                              "C.output0",
                              "C.input0",
                              "B.output0",
                              "B.input0",
                              "A.output0",
                              "A.input0" };

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
    gchar * visit_order[] = { "D.output0",
                              "D.input0",
                              "D.input1",
                              "C.output0",
                              "C.input0",
                              "B.output0",
                              "B.input0",
                              "A.output0" };

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
}

static void
bfs_property_visitor_test_setup(Test *test)
{
}

static void
bfs_property_visitor_test_teardown(Test *test)
{
}

Test *
create_bfs_property_visitor_test()
{
  Test* t = ct_create("GeglBfsPropertyVisitorTest");

  g_assert(ct_addSetUp(t, bfs_property_visitor_test_setup));
  g_assert(ct_addTearDown(t, bfs_property_visitor_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_bfs_property_visitor));
#endif

  return t;
}
