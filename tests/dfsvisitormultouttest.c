#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-dfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglNode *H,*I,*J,*K,*L,*M,*N;
static GeglNode *O,*P,*Q,*R,*S;

static void
test_dfs_visitor_mult_out_simple_chain(Test *test)
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
    gchar * visit_names[] = {"A", "B", "C"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), C); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    
    ct_test (test, 3 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }
}

static void
test_dfs_visitor_mult_out_simple_chain2(Test *test)
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
    gchar * visit_names[] = {"A", "B", "C"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), C); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 3 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }
}

static void
test_dfs_visitor_mult_out_simple_chain3(Test *test)
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
    gchar * visit_names[] = {"A", "B", "C", "D"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), D); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
  }
}

static void
test_dfs_visitor_mult_out_simple_chain4(Test *test)
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
    gchar * visit_names[] = {"A", "B", "C", "D", "E"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), E); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 5 == g_list_length(visits_list));
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(D);
    g_object_unref(E);
  }
}

#if 0
/**

  From setup: 
                   M     
                 /   \
                /     \ 
           ---------   N 
   graph L |   K   |    
           |  / \  |     
           | I   J |
           | |     |
           |null   | 
           ---------
               |
               H 

**/
static void
test_dfs_visitor_mult_out_traverse_graph(Test *test)
{
  {
    gint i;
    gint length;
    gchar * visit_names[] = {"H", "I", "J", "K", "L", "N", "M"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), M); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    length = g_list_length(visits_list);
    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

/**
    O 
    |\
    | P
    | |
    | Q
    |/ 
    R 
    |
    S 

**/
static void
test_dfs_visitor_mult_out_traverse_diamond(Test *test)
{
  {
    gint i;
    gchar * visit_names [] = {"S", "R", "Q", "P", "O"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse (GEGL_DFS_VISITOR(mock_dfs_visitor), O); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    for(i = 0; i < g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}
#endif

static void
dfs_visitor_mult_out_setup(Test *test)
{


/**
    O 
    |\
    | P 
    | |
    | Q 
    |/ 
    R 
    |
    S 

**/
  {
    O = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "O", 
                      "num_inputs", 2, 
                      NULL);  

    P = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "P", 
                      "num_inputs", 1, 
                      "num_outputs", 1, 
                      NULL);  

    Q = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "Q", 
                      "num_inputs", 1, 
                      "num_outputs", 1, 
                      NULL);  

    R = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "R", 
                      "num_inputs", 1, 
                      "num_outputs", 1, 
                      NULL);  

    S = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "S", 
                      "num_outputs", 1, 
                      NULL);  

    gegl_node_set_source_node(O, R, 0);
    gegl_node_set_source_node(O, P, 1);

    gegl_node_set_source_node(P, Q, 0);
    gegl_node_set_source_node(Q, R, 0);
    gegl_node_set_source_node(R, S, 0);
  }

  /*

             
             A   B 
             |   | 
             +   +
           --------- 
   graph C | D   E |    
           ---------

  */

  {
    H = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "H", 
                      "num_outputs", 1, 
                      NULL);  
    I = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "I", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    J = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "J", 
                      "num_outputs", 1, 
                      NULL);  
    K = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "K", 
                      "num_inputs", 2,
                      "num_outputs", 1, 
                      NULL);  
    M = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "M", 
                      "num_inputs", 2, 
                      NULL);  
    N = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "N", 
                      "num_outputs", 1, 
                      NULL);  

    gegl_node_set_source_node(K, I, 0);
    gegl_node_set_source_node(K, J, 1);
    L = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "L", 
                      "root", K, 
                      NULL);  

    gegl_node_set_source_node(L, H, 0);
    gegl_node_set_source_node(M, L, 0);
    gegl_node_set_source_node(M, N, 1);
  }

}

static void
dfs_visitor_mult_out_teardown(Test *test)
{
  g_object_unref(H);
  g_object_unref(I);
  g_object_unref(J);
  g_object_unref(K);
  g_object_unref(L);
  g_object_unref(M);
  g_object_unref(N);

  g_object_unref(O);
  g_object_unref(P);
  g_object_unref(Q);
  g_object_unref(R);
  g_object_unref(S);
}

Test *
create_dfs_visitor_mult_out_test()
{
  Test *t = ct_create("GeglDfsVisitorMultOutTest");

  g_assert(ct_addSetUp(t, dfs_visitor_mult_out_setup));
  g_assert(ct_addTearDown(t, dfs_visitor_mult_out_teardown));

  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_simple_chain));
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_simple_chain2));
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_simple_chain3));
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_simple_chain4));
#if 0 
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_traverse));
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_traverse_graph));
  g_assert(ct_addTestFun(t, test_dfs_visitor_mult_out_traverse_diamond));
#endif
  

  return t;
}
