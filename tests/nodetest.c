#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "gegl-mock-visitor.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglNode *node0, *node1, *node2, *node3;

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

    GeglNode *input0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 1,
                                    "input0", input0, 
                                    NULL);  

    GList *outputs = gegl_node_get_outputs(input0, 0); 

    g_object_unref(input0);


    ct_test(test, input0 == gegl_node_get_nth_input(node, 0)); 

    ct_test(test, 1 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(input0)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(input0)); 

    ct_test(test, node == (GeglNode*)g_list_nth_data(outputs, 0)); 
    ct_test(test, 1 == g_list_length(outputs)); 

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
    GeglConnection *input_connections = gegl_node_get_input_connections(node2);
    GeglNode * input0 = gegl_node_get_nth_input(node2, 0);
    GeglNode * input1 = gegl_node_get_nth_input(node2, 1);

    ct_test(test, 2 == gegl_node_get_num_inputs(node2));

    ct_test(test, input0 == input_connections[0].input);
    ct_test(test, 0 == input_connections[0].output_index);

    ct_test(test, input1 == input_connections[1].input);
    ct_test(test, 0 == input_connections[1].output_index);

    g_free(input_connections);
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
    GList *outputs = gegl_node_get_outputs(node0, 0);

    ct_test(test, 2 == g_list_length(outputs)); 
    ct_test(test, node2 == (GeglNode*)g_list_nth_data(outputs, 0)); 
    ct_test(test, node3 == (GeglNode*)g_list_nth_data(outputs, 1)); 
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
    GList *outputs;

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

    outputs = gegl_node_get_outputs(n0, 0); 
    ct_test(test, 1 == g_list_length(outputs)); 

    outputs = gegl_node_get_outputs(n1, 0); 
    ct_test(test, 1 == g_list_length(outputs)); 

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

}

static void
node_teardown(Test *test)
{
  g_object_unref(node0);
  g_object_unref(node1);
  g_object_unref(node2);
  g_object_unref(node3);
}

Test *
create_node_test()
{
  Test *t = ct_create("GeglNodeTest");

  g_assert(ct_addSetUp(t, node_setup));
  g_assert(ct_addTearDown(t, node_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_node_g_object_new));
  g_assert(ct_addTestFun(t, test_node_g_object_get));
  g_assert(ct_addTestFun(t, test_node_g_object_set));
  g_assert(ct_addTestFun(t, test_node_get_inputs));
  g_assert(ct_addTestFun(t, test_node_get_outputs));
  g_assert(ct_addTestFun(t, test_node_add_remove_nodes));
#endif

  return t;
}
