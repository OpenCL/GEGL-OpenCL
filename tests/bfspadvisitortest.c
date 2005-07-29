#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-pad-visitor.h"
#include "gegl-mock-operation-0-1.h"
#include "gegl-mock-operation-1-1.h"
#include "gegl-mock-operation-1-2.h"
#include "gegl-mock-operation-2-1.h"
#include "gegl-mock-operation-2-2.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>


static gboolean
do_visitor_and_check_visit_order (gchar   **visit_order,
                                  gint      length,
                                  gchar    *pad_name,
                                  GeglNode *node)
{
  gint i;
  GList *visits_list = NULL;
  GeglPad     *pad     = gegl_node_get_pad (node, pad_name);
  GeglVisitor *visitor = g_object_new (GEGL_TYPE_MOCK_PAD_VISITOR, NULL);

  gegl_visitor_bfs_traverse (visitor, GEGL_VISITABLE(pad));

  visits_list = gegl_visitor_get_visits_list (visitor);

  if (length != (gint) g_list_length (visits_list))
      return FALSE;

  for (i = 0; i < length; i++)
    {
      GeglPad       *pad       = g_list_nth_data (visits_list, i);
      GeglNode      *node      = gegl_pad_get_node (pad);
      const gchar   *node_name = gegl_object_get_name (GEGL_OBJECT (node));
      gchar         *pad_name  = g_strconcat (node_name,
                                              ".",
                                              gegl_pad_get_name(pad),
                                              NULL);

      if (0 != strcmp (pad_name, visit_order[i]))
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
test_bfs_pad_visitor(Test *test)
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

    
    GeglNode *A, *B, *C;
 
    A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation01", NULL);
    C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (C, "input0", A, "output0");
    gegl_node_connect (C, "input1", B, "output0");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 5, "output0", C));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (C, "input0", B, "output0");

    ct_test(test, do_visitor_and_check_visit_order (visit_order, 5, "output0", C));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (B, "input1", A, "output0");

    ct_test(test, do_visitor_and_check_visit_order (visit_order, 4, "output0", B));

    g_object_unref (A);
    g_object_unref (B);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (C, "input1", B, "output0");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 7, "output0", C));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation22", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (B, "input1", A, "output0");
    gegl_node_connect (C, "input0", B, "output0");
    gegl_node_connect (C, "input1", B, "output1");

    ct_test(test, do_visitor_and_check_visit_order (visit_order, 8, "output0", C));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation12", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(B, "input1", A, "output1");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 6, "output0", B));

    g_object_unref (A);
    g_object_unref (B);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (C, "input1", B, "output0");
    gegl_node_connect (C, "input0", A, "output0");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 6, "output0", C));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation11", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_NODE, "name", "D", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect (B, "input0", A, "output0");
    gegl_node_connect (C, "input0", B, "output0");
    gegl_node_connect (D, "input0", B, "output0");
    gegl_node_connect (D, "input1", C, "output0");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 9, "output0", D));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
    g_object_unref (D);
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

    GeglNode *A = g_object_new (GEGL_TYPE_NODE, "name", "A", "operation", "GeglMockOperation01", NULL);
    GeglNode *B = g_object_new (GEGL_TYPE_NODE, "name", "B", "operation", "GeglMockOperation11", NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_NODE, "name", "C", "operation", "GeglMockOperation11", NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_NODE, "name", "D", "operation", "GeglMockOperation21", NULL);

    gegl_node_connect(B, "input0", A, "output0");
    gegl_node_connect(C, "input0", B, "output0");
    gegl_node_connect(D, "input1", C, "output0");
    gegl_node_connect(D, "input0", A, "output0");

    ct_test (test, do_visitor_and_check_visit_order (visit_order, 8, "output0", D));

    g_object_unref (A);
    g_object_unref (B);
    g_object_unref (C);
    g_object_unref (D);
  }
}

static void
bfs_pad_visitor_test_setup(Test *test)
{
}

static void
bfs_pad_visitor_test_teardown(Test *test)
{
}

Test *
create_bfs_pad_visitor_test()
{
  Test* t = ct_create ("GeglBfsPropertyVisitorTest");

  
  g_assert (ct_addSetUp(t, bfs_pad_visitor_test_setup));
  g_assert (ct_addTearDown(t, bfs_pad_visitor_test_teardown));

#if 1
  g_assert (ct_addTestFun(t, test_bfs_pad_visitor));
#endif

  return t;
}
