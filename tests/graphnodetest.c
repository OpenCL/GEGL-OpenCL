#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-op.h"
#include "gegl-mock-filter.h"
#include "gegl-mock-dfs-visitor.h"
#include "gegl-mock-bfs-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglNode *A,*B,*C,*D,*E,*F,*G;
static GeglNode *H,*I,*J,*K,*L,*M,*N;

static void
test_graph_node_num_inputs(Test *test)
{
  {
    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(L)));
    ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(L)));
    ct_test(test, 2 == gegl_node_get_num_inputs(GEGL_NODE(A)));
    ct_test(test, 0 == gegl_node_get_num_outputs(GEGL_NODE(A)));
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
           | I   J |   GeglGraphInput:
           | |     |
           |null   |   node is I 
           ---------   node_input is 0
               |       graph is L
               H       graph_input is 0

**/
static void
test_graph_node_lookup_source(Test *test)
{
  {
    GeglGraphInput * source = gegl_graph_lookup_input(GEGL_GRAPH(L), I, 0);
    ct_test(test, source->node == (GeglNode*)I);
    ct_test(test, source->node_input == 0);
    ct_test(test, source->graph == (GeglGraph*)L);
    ct_test(test, source->graph_input == 0);
  }
}

/**
  From setup: 
                            two sources:
           -------------    
   graph A |   B       |    GraphInput:
           |  / \      |    node is C  
           | C   D     |    node_input is 0
           | |   |\    |    graph is A 
           |null E null|    graph_input is 0 
           -------------
               / \          GraphInput: 
              F   G         node is D 
                            node_input is 1
                            graph is A
                            graph_input is 1
**/
static void
test_graph_node_lookup_two_sources(Test *test)
{
  {
    GeglGraphInput * source = gegl_graph_lookup_input(GEGL_GRAPH(A), C, 0);
    ct_test(test, source->node == (GeglNode*)C);
    ct_test(test, source->node_input == 0);
    ct_test(test, source->graph == (GeglGraph*)A);
    ct_test(test, source->graph_input == 0);
  }

  {
    GeglGraphInput * source = gegl_graph_lookup_input(GEGL_GRAPH(A), D, 1);
    ct_test(test, source->node == (GeglNode*)D);
    ct_test(test, source->node_input == 1);
    ct_test(test, source->graph == (GeglGraph*)A);
    ct_test(test, source->graph_input == 1);
  }
}

static void
graph_node_setup(Test *test)
{
  /*
           -------------
   graph A |   B       |    
           |  / \      |     
           | C   D     |
           | |   |\    |
           |null E null| 
           -------------
               / \ 
              F   G 

  */

  {
    B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "B", 
                      "num_inputs", 2,
                      NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "C", 
                      "num_outputs", 1,
                      "num_inputs", 1,
                      NULL);  
    D = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "D", 
                      "num_outputs", 1,
                      "num_inputs", 2,
                      NULL);  
    E = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "E", 
                      "num_outputs", 1,
                      NULL);  
    F = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "F", 
                      "num_outputs", 1,
                      NULL);  
    G = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "G", 
                      "num_outputs", 1,
                      NULL);  

    gegl_node_set_source(B, C, 0);
    gegl_node_set_source(B, D, 1);
    gegl_node_set_source(D, E, 0);

    A = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "A", 
                      "root", B, 
                      NULL);  

    gegl_node_set_source(A, F, 0);
    gegl_node_set_source(A, G, 1);
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
                      "num_outputs", 1,
                      "num_inputs", 1,
                      NULL);  
    J = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "num_outputs", 1,
                      "name", "J", 
                      NULL);  
    K = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "K", 
                      "num_outputs", 1,
                      "num_inputs", 2,
                      "source-0", I,
                      "source-1", J,
                      NULL);  
    M = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "M", 
                      "num_inputs", 2, 
                      NULL);  
    N = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "name", "N", 
                      "num_outputs", 1,
                      NULL);  

    L = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "L", 
                      "root", K, 
                      NULL);  

    gegl_node_set_source(L, H, 0);
    gegl_node_set_source(M, L, 0);
    gegl_node_set_source(M, N, 1);
  }

}

static void
graph_node_teardown(Test *test)
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
}

Test *
create_graph_node_test()
{
  Test *t = ct_create("GeglGraphNodeTest");

  g_assert(ct_addSetUp(t, graph_node_setup));
  g_assert(ct_addTearDown(t, graph_node_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_graph_node_num_inputs));
  g_assert(ct_addTestFun(t, test_graph_node_lookup_source));
  g_assert(ct_addTestFun(t, test_graph_node_lookup_two_sources));
#endif

  return t;
}
