#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-bfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_bfs_visitor_mult_out_simple_chain(Test *test)
{

  /*
     
      C 
       \ 
     + +
      B  
      | 
      A 

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "A", 
                                "num_outputs", 1, 
                                NULL);  
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "B", 
                                "num_inputs", 1,
                                "num_outputs", 2, 
                                "input", 0, A,
                                NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "C", 
                                "num_inputs", 1,
                                "input", 0, B,
                                "source0_output", 1,
                                NULL);  

    gint i;
    gchar * visit_names[] = {"C", "B", "A"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), C); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    
    ct_test (test, 3 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }
}

static void
test_bfs_visitor_mult_out_simple_chain2(Test *test)
{

  /*
      C   
     / \ 
     + + 
      B
      | 
      A

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "A", 
                                "num_outputs", 1, 
                                NULL);  
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "B", 
                                "num_inputs", 1,
                                "num_outputs", 2, 
                                "input", 0, A,
                                NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "C", 
                                "num_inputs", 2,
                                "input", 0, B,
                                "source0_output", 0,
                                "input", 1, B,
                                "source1_output", 1,
                                NULL);  

    gint i;
    gchar * visit_names[] = {"C", "B", "A"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), C); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    ct_test (test, 3 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }
}

static void
test_bfs_visitor_mult_out_simple_chain3(Test *test)
{

  /*
     
      D 
     / \ 
     | C 
     | | 
     + +
      B
      | 
      A 

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "A", 
                                "num_outputs", 1, 
                                NULL);  

    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "B", 
                                "num_inputs", 1,
                                "num_outputs", 2, 
                                "input", 0, A,
                                NULL);

    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "C", 
                                "num_inputs", 1,
                                "num_outputs", 1, 
                                "input", 0, B,
                                "source0_output", 1,
                                NULL);

    GeglNode *D = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "D", 
                                "num_inputs", 2,
                                "input", 0, B,
                                "input", 1, C,
                                NULL);  

    gint i;
    gchar * visit_names[] = {"D", "C", "B", "A"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), D); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }
}

static void
test_bfs_visitor_mult_out_simple_chain4(Test *test)
{

  /*
     
      E 
     / \ 
     C D 
     | | 
     + +
      B
      | 
      A 

  */

  {
    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "A", 
                                "num_outputs", 1, 
                                NULL);  
    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "B", 
                                "num_inputs", 1,
                                "num_outputs", 2, 
                                "input", 0, A,
                                NULL);
    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "C", 
                                "num_inputs", 1,
                                "num_outputs", 1, 
                                "input", 0, B,
                                NULL);
    GeglNode *D = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "D", 
                                "num_inputs", 1,
                                "num_outputs", 1, 
                                "input", 0, B,
                                "source0_output", 1,
                                NULL);
    GeglNode *E = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                "name", "E", 
                                "num_inputs", 2,
                                "input", 0, C,
                                "input", 1, D,
                                NULL);  

    gint i;
    gchar * visit_names[] = {"E", "C", "D", "B", "A"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), E); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    ct_test (test, 5 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
    g_object_unref(E);
  }
}

static void
bfs_visitor_mult_out_setup(Test *test)
{
}

static void
bfs_visitor_mult_out_teardown(Test *test)
{
}

Test *
create_bfs_visitor_mult_out_test()
{
  Test *t = ct_create("GeglBfsVisitorMultOutTest");

  g_assert(ct_addSetUp(t, bfs_visitor_mult_out_setup));
  g_assert(ct_addTearDown(t, bfs_visitor_mult_out_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_bfs_visitor_mult_out_simple_chain));
  g_assert(ct_addTestFun(t, test_bfs_visitor_mult_out_simple_chain2));
  g_assert(ct_addTestFun(t, test_bfs_visitor_mult_out_simple_chain3));
  g_assert(ct_addTestFun(t, test_bfs_visitor_mult_out_simple_chain4));
#endif

  return t;
}
