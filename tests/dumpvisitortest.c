#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglNode *A,*B,*C,*D,*E,*F,*G;
static GeglNode *H,*I,*J,*K,*L,*M,*N;
static GeglNode *O,*P,*Q,*R,*S;

static void
test_dump_visitor_g_object_new(Test *test)
{
  {
    GeglDumpVisitor * dump_visitor = g_object_new (GEGL_TYPE_DUMP_VISITOR, NULL);  

    ct_test(test, dump_visitor != NULL);
    ct_test(test, GEGL_IS_DUMP_VISITOR(dump_visitor));
    ct_test(test, g_type_parent(GEGL_TYPE_DUMP_VISITOR) == GEGL_TYPE_VISITOR);
    ct_test(test, !strcmp("GeglDumpVisitor", g_type_name(GEGL_TYPE_DUMP_VISITOR)));

    g_object_unref(dump_visitor);
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
test_dump_visitor_traverse(Test *test)
{
  {
    GeglDumpVisitor *dump_visitor = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL);  
    gegl_dump_visitor_traverse(dump_visitor, E); 
    g_object_unref(dump_visitor);
  }

  {
    GeglDumpVisitor *dump_visitor = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL);  
    gegl_dump_visitor_traverse(dump_visitor, D); 
    g_object_unref(dump_visitor);
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
test_dump_visitor_traverse_diamond(Test *test)
{
  {
    GeglDumpVisitor *dump_visitor = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL);  
    gegl_dump_visitor_traverse (dump_visitor, O); 
    g_object_unref(dump_visitor);
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
test_dump_visitor_traverse_graph(Test *test)
{
  {
    GeglDumpVisitor *dump_visitor = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL);  
    gegl_dump_visitor_traverse(dump_visitor, M); 
    g_object_unref(dump_visitor);
  }
}

static void
dump_visitor_setup(Test *test)
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
                      "num_outputs", 1, 
                      NULL);  
    B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "B", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "C", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    D = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "D", 
                      "num_inputs", 2,
                      "num_outputs", 1, 
                      NULL);  
    E = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "E", 
                      "num_inputs", 2, 
                      NULL);  
    F = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "F", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    G = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "G", 
                      "num_outputs", 1, 
                      NULL);  


    gegl_node_set_source_node(B, A, 0);
    gegl_node_set_source_node(C, A, 0);
    gegl_node_set_source_node(D, B, 0);
    gegl_node_set_source_node(D, C, 1);
    gegl_node_set_source_node(E, D, 0);
    gegl_node_set_source_node(E, F, 1);
    gegl_node_set_source_node(F, G, 0);

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
}

static void
dump_visitor_teardown(Test *test)
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
}

Test *
create_dump_visitor_test()
{
  Test *t = ct_create("GeglDumpVisitorTest");

  g_assert(ct_addSetUp(t, dump_visitor_setup));
  g_assert(ct_addTearDown(t, dump_visitor_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_dump_visitor_g_object_new));
  g_assert(ct_addTestFun(t, test_dump_visitor_traverse));
  g_assert(ct_addTestFun(t, test_dump_visitor_traverse_diamond));
  g_assert(ct_addTestFun(t, test_dump_visitor_traverse_graph));
#endif

  return t;
}
