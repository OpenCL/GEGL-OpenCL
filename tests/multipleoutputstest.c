#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-dfs-visitor.h"
#include "gegl-mock-bfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglNode *A,*B,*C,*D,*E,*F,*G;
static GeglNode *H,*I,*J,*K,*L,*M,*N;
static GeglNode *O,*P,*Q,*R,*S;
static GeglNode *T,*U,*V,*W;

static GeglNode *a,*b,*c,*d;

/**

  From setup: 

         E 
        / \ 
       D   F 
      /\    \ 
     B  C    G 
     | /
     A 

**/
static void
test_multiple_outputs_dfs_traverse(Test *test)
{
  {
    gint i;
    gchar * visit_names[] = {"A", "B", "C", "D", "G", "F", "E"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), E); 

    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 7 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }

  {
    gint i;
    gchar * visit_names[] = { "A", "B", "C", "D" };  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), D); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

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
test_multiple_outputs_dfs_traverse_graph(Test *test)
{
  {
    gint i;
    gchar * visit_names[] = {"H", "I", "J", "K", "L", "N", "M"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), M); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 7 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
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
test_multiple_outputs_dfs_traverse_diamond(Test *test)
{
  {
    gint i;
    gchar * visit_names [] = {"S", "R", "Q", "P", "O"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse (GEGL_DFS_VISITOR(mock_dfs_visitor), O); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 5 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

/**
    d 
   / \ 
   |  c 
   \ / 
    b 
    | 
    a 
**/
static void
test_multiple_outputs_dfs_simple_shared(Test *test)
{

  {
    gint i;
    gchar * visit_names[] = {"a", "b", "c", "d"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), d); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

/**

  From setup: 

          T 
         /|\ 
        / | \ 
       U null V  
      /|
     W null

**/
static void
test_multiple_outputs_dfs_traverse_with_nulls(Test *test)
{
  {
    gint i;
    gchar * visit_names[] = {"W", "U", "V", "T"};  
    GeglMockDfsVisitor *mock_dfs_visitor = g_object_new(GEGL_TYPE_MOCK_DFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(mock_dfs_visitor), T); 

    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_dfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_dfs_visitor);
  }
}

/**
  From node_setup: 

         E 
        / \ 
       D   F 
      /\    \ 
     B  C    G 
     | /
     A 

**/
static void
test_multiple_outputs_bfs_traverse(Test *test)
{
  {
    gint i;
    gchar * visit_names [] = { "E", "D", "F", "B", "C", "G", "A" };  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse (GEGL_BFS_VISITOR(mock_bfs_visitor), E); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);  
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
  }

  {
    gint i;
    gchar * visit_names[] = { "D", "B", "C", "A" };  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse (GEGL_BFS_VISITOR(mock_bfs_visitor), D); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);  
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
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
test_multiple_outputs_bfs_traverse_diamond(Test *test)
{
  {
    gint i;
    gchar * visit_names [] = {"O", "P", "Q", "R", "S"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse (GEGL_BFS_VISITOR(mock_bfs_visitor), O); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);  
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
  }
}

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
test_multiple_outputs_bfs_traverse_graph(Test *test)
{
  {
    gint i;
    gint length;
    gchar * visit_names[] = {"M", "L", "K", "I", "J", "N", "H"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), M); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    length = g_list_length(visits_list);
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);  
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
  }
}

static void
test_multiple_outputs_bfs_simple_shared(Test *test)
{
  {
    gint i;
    gchar * visit_names[] = {"d", "c", "b", "a"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), d); 
    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
  }
}

/**

  From setup: 

          T 
         /|\ 
        / | \ 
       U null V  
      /|
     W null

**/
static void
test_multiple_outputs_bfs_traverse_with_nulls(Test *test)
{
  {
    gint i;
    gchar * visit_names[] = {"T", "U", "V", "W"};  
    GeglMockBfsVisitor *mock_bfs_visitor = g_object_new(GEGL_TYPE_MOCK_BFS_VISITOR, NULL);  
    GList * visits_list;

    gegl_bfs_visitor_traverse(GEGL_BFS_VISITOR(mock_bfs_visitor), T); 

    visits_list = gegl_visitor_get_visits_list(GEGL_VISITOR(mock_bfs_visitor));

    ct_test (test, 4 == g_list_length(visits_list));
    for(i = 0; i < (gint)g_list_length(visits_list); i++)
      {
        GeglNode *node = (GeglNode*)g_list_nth_data(visits_list, i);
        const gchar *name = gegl_object_get_name(GEGL_OBJECT(node));
        ct_test (test, 0 == strcmp(name, visit_names[i]));
      }

    g_object_unref(mock_bfs_visitor);
  }
}



static void
multiple_outputs_setup(Test *test)
{

  /*
         E 
        / \ 
       D   F 
      /\    \ 
     B  C    G 
     | /
     A 

  */

  {
    A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "A", 
                      "num_outputs", 2,  
                      NULL);  
    B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "B", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "C", 
                      "num_inputs", 1,
                      "num_outputs", 2, 
                      NULL);  
    D = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "D", 
                      "num_inputs", 2,
                      "num_outputs", 2, 
                      NULL);  
    E = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "E", 
                      "num_inputs", 2, 
                      NULL);  
    F = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "F", 
                      "num_inputs", 1,
                      "num_outputs", 2,  
                      NULL);  
    G = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "G", 
                      "num_outputs", 3, 
                      NULL);  

    gegl_node_set_m_source(B, A, 0, 0); /*0th input B, 0th output A*/
    gegl_node_set_m_source(C, A, 0, 1); /*0th input C, 1st output A*/
    gegl_node_set_m_source(D, B, 0, 0); /*1st input D, 0th output B*/
    gegl_node_set_m_source(D, C, 1, 1); /*1st input D, 1st output C*/
    gegl_node_set_m_source(E, D, 0, 1); /*0th input E, 1st output D*/
    gegl_node_set_m_source(E, F, 1, 1); /*1st input E, 1st output F*/
    gegl_node_set_m_source(F, G, 0, 2); /*0th input F, 2nd output G*/
  }

  /*
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

  */

  {
    H = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "H", 
                      "num_outputs", 2, 
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
                      "num_outputs", 2, 
                      NULL);  
    M = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "M", 
                      "num_inputs", 2, 
                      NULL);  
    N = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "N", 
                      "num_outputs", 1, 
                      NULL);  

    gegl_node_set_m_source(K, I, 0, 0);
    gegl_node_set_m_source(K, J, 1, 0);
    L = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "L", 
                      "root", K, 
                      NULL);  

    gegl_node_set_m_source(L, H, 0, 1);
    gegl_node_set_m_source(M, L, 0, 1);
    gegl_node_set_m_source(M, N, 1, 0);
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
                      "num_outputs", 2, 
                      NULL);  

    S = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "S", 
                      "num_outputs", 2, 
                      NULL);  

    gegl_node_set_m_source(O, R, 0, 0);
    gegl_node_set_m_source(O, P, 1, 0);
    gegl_node_set_m_source(P, Q, 0, 0);
    gegl_node_set_m_source(Q, R, 0, 1);
    gegl_node_set_m_source(R, S, 0, 1);
  }

  /*
          T 
         /| \ 
        / |  \ 
       U null V  
      /|
     W null
  */

  {
    W = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "W", 
                      "num_outputs", 2,
                      NULL);  

    V = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "V", 
                      "num_outputs", 1,
                      NULL);  

    U = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "U", 
                      "num_outputs", 2,
                      "num_inputs", 2,
                      "m-source-0", W, 0,
                      "m-source-1", NULL, 0,
                      NULL);  

    T = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "T", 
                      "num_inputs", 3,
                      "m-source-0", U, 1,
                      "m-source-1", NULL, 0,
                      "m-source-2", V, 0,
                      NULL);  
  }


  /*
      d 
     / \ 
     |  c 
     \ / 
      b 
      | 
      a 
  */

  {
    a = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "a", 
                      "num_outputs", 2, 
                      NULL);  
    b = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "b", 
                      "num_inputs", 1,
                      "num_outputs", 2, 
                      "m-source-0", a, 1,
                      NULL);

    c = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "c", 
                      "num_inputs", 1,
                      "num_outputs", 2, 
                      "m-source-0", b, 0,
                      NULL);

    d = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "d", 
                      "num_inputs", 2,
                      "m-source-0", b, 1,
                      "m-source-1", c, 1,
                      NULL);  

  }

}

static void
multiple_outputs_teardown(Test *test)
{
  g_object_unref(A);
  g_object_unref(B);
  g_object_unref(C);
  g_object_unref(D);
  g_object_unref(E);
  g_object_unref(F);
  g_object_unref(G);
  
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

  g_object_unref(T);
  g_object_unref(U);
  g_object_unref(V);
  g_object_unref(W);

  g_object_unref(a);
  g_object_unref(b);
  g_object_unref(c);
  g_object_unref(d);
}

Test *
create_multiple_outputs_test()
{
  Test *t = ct_create("GeglMultipleOutputsTest");

  g_assert(ct_addSetUp(t, multiple_outputs_setup));
  g_assert(ct_addTearDown(t, multiple_outputs_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_multiple_outputs_dfs_traverse));
  g_assert(ct_addTestFun(t, test_multiple_outputs_dfs_traverse_graph));
  g_assert(ct_addTestFun(t, test_multiple_outputs_dfs_traverse_diamond));
  g_assert(ct_addTestFun(t, test_multiple_outputs_dfs_simple_shared));
  g_assert(ct_addTestFun(t, test_multiple_outputs_dfs_traverse_with_nulls));

  g_assert(ct_addTestFun(t, test_multiple_outputs_bfs_traverse));
  g_assert(ct_addTestFun(t, test_multiple_outputs_bfs_traverse_graph));
  g_assert(ct_addTestFun(t, test_multiple_outputs_bfs_traverse_diamond));
  g_assert(ct_addTestFun(t, test_multiple_outputs_bfs_simple_shared));
  g_assert(ct_addTestFun(t, test_multiple_outputs_bfs_traverse_with_nulls));
#endif
  
  return t;
}
