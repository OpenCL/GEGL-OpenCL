#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglNode *A,*B,*C,*D,*E,*F,*G;
static GeglNode *H,*I,*J,*K,*L,*M,*N;

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
           ---------   node_input_index is 0
               |       graph is L
               H       graph_input_index is 0

**/
static void
test_graph_node_lookup_input(Test *test)
{
  g_print("test_graph_node_lookup_input\n");
  {
    GeglGraphInput * input = gegl_graph_lookup_input(GEGL_GRAPH(L), I, 0);
    ct_test(test, input->node == (GeglNode*)I);
    ct_test(test, input->node_input_index == 0);
    ct_test(test, input->graph == (GeglGraph*)L);
    ct_test(test, input->graph_input_index == 0);
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
           | I   J |   GeglGraphOutput:
           | |     |   
           |null   |   node is K 
           ---------   node_output_index is 0
               |       graph is L
               H       graph_output_index is 0

**/
static void
test_graph_node_lookup_output(Test *test)
{
  g_print("test_graph_node_lookup_output\n");
  {
    GeglGraphOutput * output = gegl_graph_lookup_output(GEGL_GRAPH(L), 0);
    ct_test(test, output->node == (GeglNode*)K);
    ct_test(test, output->node_output_index == 0);
    ct_test(test, output->graph == (GeglGraph*)L);
    ct_test(test, output->graph_output_index == 0);
  }
}

/**
  From setup: 
                            two inputs:
           -------------    
   graph A |   B       |    GraphInput:
           |  / \      |    node is C  
           | C   D     |    node_input_index is 0
           | |   |\    |    graph is A 
           |null E null|    graph_input_index is 0 
           -------------
               / \          GraphInput: 
              F   G         node is D 
                            node_input_index is 1
                            graph is A
                            graph_input_index is 1
**/
static void
test_graph_node_lookup_two_inputs(Test *test)
{
  g_print("test_graph_node_lookup_two_inputs\n");
  {
    GeglGraphInput * input = gegl_graph_lookup_input(GEGL_GRAPH(A), C, 0);
    ct_test(test, input->node == (GeglNode*)C);
    ct_test(test, input->node_input_index == 0);
    ct_test(test, input->graph == (GeglGraph*)A);
    ct_test(test, input->graph_input_index == 0);
  }

  {
    GeglGraphInput * input = gegl_graph_lookup_input(GEGL_GRAPH(A), D, 1);
    ct_test(test, input->node == (GeglNode*)D);
    ct_test(test, input->node_input_index == 1);
    ct_test(test, input->graph == (GeglGraph*)A);
    ct_test(test, input->graph_input_index == 1);
  }
}

/**
  From setup: 
                            
           -------------    
   graph A |   B       |    
           |  / \      |   
           | C   D     |   
           | |   |\    |    
           |null E null|   
           -------------
               / \         
              F   G         
                            
**/
static void
test_graph_init_visitor(Test *test)
{
  {
    GeglGraph *graph = g_object_new (GEGL_TYPE_GRAPH, 
                                     "name", "Graph", 
                                     "root", A, 
                                     NULL);  

    GeglGraphInitVisitor * graph_init_visitor = 
      g_object_new(GEGL_TYPE_GRAPH_INIT_VISITOR, NULL); 

    graph_init_visitor->graph = graph;

    gegl_dfs_visitor_traverse(GEGL_DFS_VISITOR(graph_init_visitor), 
                              GEGL_NODE(graph));

    {
      GeglDumpVisitor *dump_visitor = g_object_new(GEGL_TYPE_DUMP_VISITOR, NULL);  
      gegl_dump_visitor_traverse(dump_visitor, GEGL_GRAPH(A)->root); 
      g_object_unref(dump_visitor);
    }

    g_object_unref(graph_init_visitor);
    g_object_unref(graph);
  }
}

static void
graph_node_setup(Test *test)
{
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
    H = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "H", 
                      "num_outputs", 1, 
                      NULL);  
    I = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "I", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    J = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "J", 
                      "num_outputs", 1, 
                      NULL);  
    K = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "K", 
                      "num_inputs", 2,
                      "num_outputs", 1, 
                      NULL);  
    M = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "M", 
                      "num_inputs", 2, 
                      NULL);  
    N = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "N", 
                      "num_outputs", 1, 
                      NULL);  

    gegl_node_set_nth_input(K, I, 0);
    gegl_node_set_nth_input(K, J, 1);
    L = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "L", 
                      "root", K, 
                      NULL);  

    gegl_node_set_nth_input(L, H, 0);
    gegl_node_set_nth_input(M, L, 0);
    gegl_node_set_nth_input(M, N, 1);
  }

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
    B = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "B", 
                      "num_inputs", 2,
                      "num_outputs", 1, 
                      NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "C", 
                      "num_inputs", 1,
                      "num_outputs", 1, 
                      NULL);  
    D = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "D", 
                      "num_inputs", 2,
                      "num_outputs", 1, 
                      NULL);  
    E = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "E", 
                      "num_outputs", 1, 
                      NULL);  
    F = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "F", 
                      "num_outputs", 1, 
                      NULL);  
    G = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                      "name", "G", 
                      "num_outputs", 1, 
                      NULL);  

    gegl_node_set_nth_input(B, C, 0);
    gegl_node_set_nth_input(B, D, 1);
    gegl_node_set_nth_input(D, E, 0);

    A = g_object_new (GEGL_TYPE_GRAPH, 
                      "name", "A", 
                      "root", B, 
                      NULL);  

    gegl_node_set_nth_input(A, F, 0);
    gegl_node_set_nth_input(A, G, 1);
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
  g_assert(ct_addTestFun(t, test_graph_init_visitor));
  g_assert(ct_addTestFun(t, test_graph_node_lookup_input));
  g_assert(ct_addTestFun(t, test_graph_node_lookup_output));
  g_assert(ct_addTestFun(t, test_graph_node_lookup_two_inputs));
#endif

  return t;
}
