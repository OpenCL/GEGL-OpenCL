#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static GeglNode *node0, *node1, *node2, *node3;

static void
test_node_g_object_new(Test *test)
{
  {
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    NULL);  

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

       A
      / \
     B   C

    */
    
    GeglNode *C, *B, *A;
    B = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "num_outputs", 1,
                      NULL);  
    C = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "num_outputs", 1,
                      NULL);  

    A = g_object_new (GEGL_TYPE_MOCK_NODE, 
                      "num_inputs", 2,
                      "input", 0, B, 
                      "input", 1, C, 
                      NULL);  

    ct_test(test, 0 == gegl_node_get_num_outputs(A)); 
    ct_test(test, 2 == gegl_node_get_num_inputs(A)); 

    ct_test(test, 1 == gegl_node_get_num_outputs(B)); 
    ct_test(test, 0 == gegl_node_get_num_inputs(B)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(B)); 

    ct_test(test, 1 == gegl_node_get_num_outputs(C)); 
    ct_test(test, 0 == gegl_node_get_num_inputs(C)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(C)); 

    ct_test(test, B == gegl_node_get_source(A, 0)); 
    ct_test(test, C == gegl_node_get_source(A, 1)); 

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
  }

  {
    /*
                       
       node
        |   
      source0 

    */

    GeglNode *source0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 1,
                                    "input", 0, source0, 
                                    NULL);  

    g_object_unref(source0);

    /* node should still has a ref to source0 */

    ct_test(test, source0 == gegl_node_get_source(node, 0)); 

    ct_test(test, 1 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(source0)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(source0)); 

    ct_test(test, source0 == gegl_node_get_source(node, 0)); 

    ct_test(test, 1 == gegl_node_get_num_sinks(source0)); 

    g_object_unref(node);
  }
}

static void
test_node_g_object_get(Test *test)
{
  /*
     from setup:

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

#if 0
  {
    GeglNode * source0 = NULL; 
    GeglNode * source1 = NULL; 

    g_object_get(node2, "source0", &source0, NULL);
    ct_test(test, source0 == node0);

    g_object_get(node2, "source1", &source1, NULL);
    ct_test(test, source1 == node1);

    g_object_unref(source0);
    g_object_unref(source1);
  }
#endif
}


static void
test_node_g_object_set(Test *test)
{
  {
    GeglNode *source0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode *source1 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    g_object_set(node, 
                 "input", 0, source0, 
                 "input", 1, source1, 
                 NULL);

    ct_test(test, source0 == gegl_node_get_source(node, 0)); 
    ct_test(test, source1 == gegl_node_get_source(node, 1)); 

    g_object_unref(node);
    g_object_unref(source0);
    g_object_unref(source1);
  }
}

static void
test_node_get_sources(Test *test)
{
    /*
     from setup:

      node2 node3   <---- node2 has 2 inputs, node3 has 1 input. 
       - -  -
       |  \/ 
       |  /\  
       | /  \  
       +     +       <---- node0, node1 each have one output.
      node0 node1     

    */

  {
    GeglNode *source0, *source1;
    source0 = gegl_node_get_source(node2, 0);
    source1 = gegl_node_get_source(node2, 1);

    ct_test(test, 2 == gegl_node_get_num_inputs(node2));
  }
}

static void
test_node_get_sinks(Test *test)
{
    /*
     from setup:

      node2 node3   <---- node2 has 2 inputs, node3 has 1 input. 
       - -  -
       |  \/ 
       |  /\  
       | /  \  
       +     +       <---- node0, node1 each have one output.
      node0 node1   

    */
  {
    ct_test(test, 2 == gegl_node_get_num_sinks(node0)); 

    ct_test(test, node2 == gegl_node_get_sink(node0, 0)); 
    ct_test(test, 0 == gegl_node_get_sink_input(node0, 0)); 

    ct_test(test, node3 == gegl_node_get_sink(node0, 1)); 
    ct_test(test, 0 == gegl_node_get_sink_input(node0, 1)); 

    ct_test(test, 1 == gegl_node_get_num_sinks(node1)); 

    ct_test(test, node2 == gegl_node_get_sink(node1, 0)); 
    ct_test(test, 1 == gegl_node_get_sink_input(node1, 0)); 
  }
}

static void
test_node_set_source_node(Test *test)
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

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    gegl_node_set_source (n2, n0, 0);
    gegl_node_set_source (n3, n0, 0);

    gegl_node_set_source (n2, n1, 1);
    gegl_node_set_source (n3, n1, 1);

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

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1)); 

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

    gegl_node_set_source (n2, n0, 0);
    gegl_node_set_source (n3, n0, 0);

    gegl_node_set_source (n2, n1, 1);
    gegl_node_set_source (n3, n1, 1);

    g_object_unref(n0); 
    g_object_unref(n1);

    
    /*
         n2  n3  Remove n0 
          \   |
           \  |
            \ |
             n1 
    */

    gegl_node_set_source (n2, NULL, 0); 
    gegl_node_set_source (n3, NULL, 0); 

    ct_test(test, 2 == gegl_node_get_num_inputs(n2)); 
    ct_test(test, NULL == gegl_node_get_source (n2, 0)); 
    ct_test(test, n1 == gegl_node_get_source (n2, 1)); 

    ct_test(test, 2 == gegl_node_get_num_inputs(n3)); 
    ct_test(test, NULL == gegl_node_get_source (n3, 0)); 
    ct_test(test, n1 == gegl_node_get_source (n3, 1)); 

    g_object_unref(n2);
    g_object_unref(n3);

  }
}

static void
test_node_unlink(Test *test)
{
  {
    /*
       n1
       |
       n0
    */

    GeglNode *n0, *n1;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "input", 0, n0, 
                       NULL);  

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, n0 == gegl_node_get_source (n1, 0)); 

    gegl_node_unlink(n0);

    ct_test(test, 0 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, NULL == gegl_node_get_source (n1, 0)); 

    g_object_unref(n1);
    g_object_unref(n0); 
  }
#if 0
  {
    /*
       n2
       |
       n1
       |
       n0
    */

    GeglNode *n0, *n1, *n2;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       "num_inputs", 1, 
                       "input", 0, n0, 
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "input", 0, n1, 
                       NULL);  

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, n1 == gegl_node_get_source (n2, 0)); 
    ct_test(test, n0 == gegl_node_get_source (n1, 0)); 

    gegl_node_unlink(n1);

    ct_test(test, 0 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, NULL == gegl_node_get_source (n2, 0)); 
    ct_test(test, NULL == gegl_node_get_source (n1, 0)); 

    g_object_unref(n1);

    g_object_unref(n0); 
    g_object_unref(n2);
  }
#endif
}

static void
test_node_remove_sources(Test *test)
{
  {
    /*
       n2
       |
       n1
       |
       n0
    */

    GeglNode *n0, *n1, *n2;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       "num_inputs", 1, 
                       "input", 0, n0, 
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "input", 0, n1, 
                       NULL);  

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, n1 == gegl_node_get_source(n2, 0)); 
    ct_test(test, n0 == gegl_node_get_source(n1, 0)); 

    gegl_node_remove_sources(n2);

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, NULL == gegl_node_get_source(n2, 0)); 
    ct_test(test, n0 == gegl_node_get_source(n1, 0)); 

    g_object_unref(n2);
    g_object_unref(n1);
    g_object_unref(n0); 
  }
}

static void
test_node_remove_sinks(Test *test)
{
  {
    /*
       n2
       |
       n1
       |
       n0
    */

    GeglNode *n0, *n1, *n2;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 1, 
                       "num_inputs", 1, 
                       "input", 0, n0, 
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "input", 0, n1, 
                       NULL);  

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, n1 == gegl_node_get_source(n2, 0)); 
    ct_test(test, n0 == gegl_node_get_source(n1, 0)); 

    gegl_node_remove_sinks(n1);

    ct_test(test, 1 == gegl_node_get_num_sinks(n0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1)); 
    ct_test(test, NULL == gegl_node_get_source(n2, 0)); 
    ct_test(test, n0 == gegl_node_get_source(n1, 0)); 

    g_object_unref(n2);
    g_object_unref(n1);
    g_object_unref(n0); 
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
     +     +      <---- node0, node1 each have one output.
    node0 node1   

  */

  node0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  
  node1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 1, NULL);  

  node2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 2,
                        "input", 0, node0,
                        "input", 1, node1, 
                        NULL);  

  node3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 1,
                        "input", 0, node0,
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
  g_assert(ct_addTestFun(t, test_node_get_sources));
  g_assert(ct_addTestFun(t, test_node_get_sinks));
  g_assert(ct_addTestFun(t, test_node_set_source_node));
  g_assert(ct_addTestFun(t, test_node_unlink));
  g_assert(ct_addTestFun(t, test_node_remove_sources));
  g_assert(ct_addTestFun(t, test_node_remove_sinks));
#endif

  return t;
}
