#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglNode *node0, *node1, *node2, *node3;
static GeglNode *A,*B,*C,*D,*E,*F,*G,*H;
static gint traversal_count;

static void
test_node_g_object_new(Test *test)
{
  {
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, NULL);  

    ct_test(test, node != NULL);
    ct_test(test, GEGL_IS_NODE(node));
    ct_test(test, g_type_parent(GEGL_TYPE_NODE) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglNode", g_type_name(GEGL_TYPE_NODE)));

    ct_test(test, GEGL_IS_MOCK_NODE(node));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_NODE) == GEGL_TYPE_NODE);
    ct_test(test, !strcmp("GeglMockNode", g_type_name(GEGL_TYPE_MOCK_NODE)));

    ct_test(test, 0 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(node)); 
    g_object_unref(node);
  }


  {
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 2,
                                    "num_outputs", 1,
                                    NULL);  

    ct_test(test, 2 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(node)); 

    g_object_unref(node);
  }

  {
    /*
                       
       node
        |   
      input0 

    */

    gint index_num_outputs;
    GeglNode *input0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 1,
                                    "input0", input0, 
                                    NULL);  

    GeglNode **outputs = gegl_node_get_outputs(input0, &index_num_outputs, 0); 

    g_object_unref(input0);


    ct_test(test, input0 == gegl_node_get_nth_input(node, 0)); 

    ct_test(test, 1 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(input0)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(input0)); 

    ct_test(test, node == outputs[0]); 
    ct_test(test, 1 == index_num_outputs); 

    g_free(outputs);
    g_object_unref(node);
  }

  {
    GeglNode *input0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode *input1 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);

    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 2,
                                    "num_outputs", 1,
                                    "input0", input0, 
                                    "input1", input1, 
                                    NULL);  

    g_object_unref(input0);
    g_object_unref(input1);

    /*

         node
        /   \
     input0 input1

    */

    ct_test(test, input0 == gegl_node_get_nth_input(node, 0)); 
    ct_test(test, input1 == gegl_node_get_nth_input(node, 1)); 

    ct_test(test, 2 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(input0)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(input0)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(input1)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(input1)); 

    g_object_unref(node);
  }
}

static void
test_node_g_object_get(Test *test)
{
  /*

    node2 node3   node3 num_inputs = 1
     - -  -       node2 num_inputs = 2
     |  \/ 
     |  /\  
     | /  \  
     +     +      node1 num_outputs = 1
    node0 node1   node0 num_outputs = 1

  */

  {
    gint num_inputs; 
    gint num_outputs; 

    g_object_get(node2, "num_inputs", &num_inputs, NULL);
    ct_test(test, 2 == num_inputs);

    g_object_get(node0, "num_outputs", &num_outputs, NULL);
    ct_test(test, 1 == num_outputs);
  }


  {
    GeglNode * input0 = NULL; 
    GeglNode * input1 = NULL; 

    g_object_get(node2, "input0", &input0, NULL);
    ct_test(test, input0 == node0);

    g_object_get(node2, "input1", &input1, NULL);
    ct_test(test, input1 == node1);

    g_object_unref(input0);
    g_object_unref(input1);
  }
}


static void
test_node_g_object_set(Test *test)
{
  {
    GeglNode *input0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode *input1 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    g_object_set(node, 
                 "input0", input0, 
                 "input1", input1, 
                 NULL);

    ct_test(test, input0 == gegl_node_get_nth_input(node, 0)); 
    ct_test(test, input1 == gegl_node_get_nth_input(node, 1)); 

    g_object_unref(node);
    g_object_unref(input0);
    g_object_unref(input1);
  }
}

static void
test_node_get_inputs(Test *test)
{
    /*

      node2 node3   <---- node2 has 2 inputs, node3 has 1 input. 
       - -  -
       |  \/ 
       |  /\  
       | /  \  
       +     +       <---- node0, node1 have one output index.
      node0 node1   

    */

  {
    GeglInputInfo *input_infos = gegl_node_get_input_infos(node2);
    GeglNode * input0 = gegl_node_get_nth_input(node2, 0);
    GeglNode * input1 = gegl_node_get_nth_input(node2, 1);

    ct_test(test, 2 == gegl_node_get_num_inputs(node2));

    ct_test(test, input0 == input_infos[0].input);
    ct_test(test, 0 == input_infos[0].index);

    ct_test(test, input1 == input_infos[1].input);
    ct_test(test, 0 == input_infos[1].index);

    g_free(input_infos);
  }
}

static void
test_node_get_outputs(Test *test)
{
    /*

      node2 node3   <---- node2 has 2 inputs, node3 has 1 input. 
       - -  -
       |  \/ 
       |  /\  
       | /  \  
       +     +       <---- node0, node1 have one output index.
      node0 node1   

    */
  {
    gint index_num_outputs;
    GeglNode **outputs = gegl_node_get_outputs(node0, &index_num_outputs, 0);

    ct_test(test, 2 == index_num_outputs); 
    ct_test(test, node2 == outputs[0]); 
    ct_test(test, node3 == outputs[1]); 

    g_free(outputs);
  }
}

static void
test_node_add_remove_nodes(Test *test)
{
  {
    /*
         n2  n3 
         |\  /|
         | \/ |
         | /\ |
         n0  n1 
    */

    GeglNode *n0, *n1, *n2, *n3;
    gint index_num_outputs;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    gegl_node_set_nth_input(n2, n0, 0);
    gegl_node_set_nth_input(n3, n0, 0);

    gegl_node_set_nth_input(n2, n1, 1);
    gegl_node_set_nth_input(n3, n1, 1);

    g_object_unref(n0); 
    g_object_unref(n1);

    
    /*
         n2    Remove n3 
         |\  
         | \
         |  \ 
         n0  n1 
    */

    g_object_unref(n3);

    gegl_node_get_outputs(n0, &index_num_outputs, 0); 
    ct_test(test, 1 == index_num_outputs); 

    gegl_node_get_outputs(n1, &index_num_outputs, 0); 
    ct_test(test, 1 == index_num_outputs); 

    /*
       Remove n2 
    */

    g_object_unref(n2);
  }

  {
    /*
         n2  n3 
         |\  /|
         | \/ |
         | /\ |
         n0  n1 
    */

    GeglNode *n0, *n1, *n2, *n3;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    gegl_node_set_nth_input(n2, n0, 0);
    gegl_node_set_nth_input(n3, n0, 0);

    gegl_node_set_nth_input(n2, n1, 1);
    gegl_node_set_nth_input(n3, n1, 1);

    g_object_unref(n0); 
    g_object_unref(n1);

    
    /*
         n2  n3  Remove n0 
          \   |
           \  |
            \ |
             n1 
    */

    gegl_node_set_nth_input(n2, NULL, 0); 
    gegl_node_set_nth_input(n3, NULL, 0); 

    ct_test(test, 2 == gegl_node_get_num_inputs(n2)); 
    ct_test(test, NULL == gegl_node_get_nth_input(n2, 0)); 
    ct_test(test, n1 == gegl_node_get_nth_input(n2, 1)); 

    ct_test(test, 2 == gegl_node_get_num_inputs(n3)); 
    ct_test(test, NULL == gegl_node_get_nth_input(n3, 0)); 
    ct_test(test, n1 == gegl_node_get_nth_input(n3, 1)); 

    g_object_unref(n2);
    g_object_unref(n3);

  }
}


gboolean 
traverse_depth_first_whole_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "A", "B", "C", "D", "G", "F", "E" };  
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[traversal_count]));
  traversal_count++;
  return TRUE;
} 

gboolean 
traverse_depth_first_part_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "A", "B", "C", "D" };  
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[traversal_count]));
  traversal_count++;
  return TRUE;
} 

/**
  From node_setup: 

         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

**/
static void
test_depth_first_traversal(Test *test)
{
  traversal_count = 0;
  gegl_node_traverse_depth_first (E, traverse_depth_first_whole_graph, test, TRUE); 
  traversal_count = 0;
  gegl_node_traverse_depth_first (D, traverse_depth_first_part_graph, test, TRUE);
}


gboolean 
traverse_breadth_first_whole_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "E", "D", "F", "B", "C", "G", "A" };  
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[traversal_count]));
  traversal_count++;
  return TRUE;
} 

gboolean 
traverse_breadth_first_part_graph(GeglNode *node, gpointer data)
{
  Test * test = (Test*) data;
  char * names[] = { "D", "B", "C", "A" };  
  ct_test (test, 0 == strcmp(g_object_get_data(G_OBJECT(node), "name"), names[traversal_count]));
  traversal_count++;
  return TRUE;
} 

/**
  From node_setup: 

         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

**/
static void
test_breadth_first_traversal(Test *test)
{
  traversal_count = 0;
  gegl_node_traverse_breadth_first (E, traverse_breadth_first_whole_graph, test); 
  traversal_count = 0;
  gegl_node_traverse_breadth_first (D, traverse_breadth_first_part_graph, test); 
}

static void
node_setup(Test *test)
{

  /*

    node2 node3   <---- node2 has 2 inputs, node3 has 1 input. 
     - -  -
     |  \/ 
     |  /\  
     | /  \  
     +     +       <---- node0, node1 have one output index.
    node0 node1   

  */

  node0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
  node1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  

  node2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 2,
                        "input0", node0,
                        "input1", node1, 
                        NULL);  

  node3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 1,
                        "input0", node0,
                        NULL);  

  /*
         E 
        / \ 
       D   F 
      /\    \ 
  H  B  C    G 
   \ | /
     A 

  */

  {
    A = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    B = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1,"num_outputs", 1, NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1,"num_outputs", 1, NULL);  
    D = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2,"num_outputs", 1, NULL);  
    E = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    F = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1,"num_outputs", 1, NULL);  
    G = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    H = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1, NULL);  


    g_object_set_data(G_OBJECT(A), "name", "A");
    g_object_set_data(G_OBJECT(B), "name", "B");
    g_object_set_data(G_OBJECT(C), "name", "C");
    g_object_set_data(G_OBJECT(D), "name", "D");
    g_object_set_data(G_OBJECT(E), "name", "E");
    g_object_set_data(G_OBJECT(F), "name", "F");
    g_object_set_data(G_OBJECT(G), "name", "G");
    g_object_set_data(G_OBJECT(H), "name", "H");

    gegl_node_set_nth_input(B, A, 0);
    gegl_node_set_nth_input(C, A, 0);
    gegl_node_set_nth_input(D, B, 0);
    gegl_node_set_nth_input(D, C, 1);
    gegl_node_set_nth_input(E, D, 0);
    gegl_node_set_nth_input(E, F, 1);
    gegl_node_set_nth_input(F, G, 0);
    gegl_node_set_nth_input(H, A, 0);

  }
}

static void
node_teardown(Test *test)
{
  g_object_unref(node0);
  g_object_unref(node1);
  g_object_unref(node2);
  g_object_unref(node3);

  g_object_unref(A);
  g_object_unref(B);
  g_object_unref(C);
  g_object_unref(D);
  g_object_unref(E);
  g_object_unref(F);
  g_object_unref(G);
  g_object_unref(H);

}

Test *
create_node_test()
{
  Test *t = ct_create("GeglNodeTest");

  g_assert(ct_addSetUp(t, node_setup));
  g_assert(ct_addTearDown(t, node_teardown));


  g_assert(ct_addTestFun(t, test_node_g_object_new));
  g_assert(ct_addTestFun(t, test_node_g_object_get));
  g_assert(ct_addTestFun(t, test_node_g_object_set));
  g_assert(ct_addTestFun(t, test_node_get_inputs));
  g_assert(ct_addTestFun(t, test_node_get_outputs));
  g_assert(ct_addTestFun(t, test_node_add_remove_nodes));
  g_assert(ct_addTestFun(t, test_depth_first_traversal));
  g_assert(ct_addTestFun(t, test_breadth_first_traversal));

  return t;
}
