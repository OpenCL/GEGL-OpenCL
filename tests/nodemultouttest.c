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
test_node_mult_out_g_object_new(Test *test)
{
  {
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_outputs", 2,
                                    NULL);  

    ct_test(test, 0 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 2 == gegl_node_get_num_outputs(node)); 

    g_object_unref(node);
  }

  {
    /*
                       
       node     <---one input 
          \   
        +  +
       source0   <--- two outputs


      Here source0 has one sink attached to output 1. 

    */

    GeglNode *source0 = g_object_new(GEGL_TYPE_MOCK_NODE, 
                                     "num_outputs", 2, 
                                     NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 1,
                                    "source0", source0, 
                                    "source0_output", 1, 
                                    NULL);  

    ct_test(test, source0 == gegl_node_get_source_node(node, 0)); 
    ct_test(test, 1 == gegl_node_get_source_output(node, 0)); 

    ct_test(test, 1 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(source0)); 
    ct_test(test, 2 == gegl_node_get_num_outputs(source0)); 

    ct_test(test, 0 == gegl_node_get_num_sinks(source0, 0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(source0, 1)); 

    g_object_unref(source0);
    g_object_unref(node);
  }

  {
    GeglNode *source0 = g_object_new(GEGL_TYPE_MOCK_NODE, 
                                     "num_outputs", 2, 
                                     NULL);
    GeglNode *source1 = g_object_new(GEGL_TYPE_MOCK_NODE, 
                                     "num_outputs", 2, 
                                     NULL);

    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, 
                                    "num_inputs", 2,
                                    "num_outputs", 1,
                                    "source0", source0, 
                                    "source0_output", 1, 
                                    "source1", source1, 
                                    "source1_output", 0, 
                                    NULL);  

    g_object_unref(source0);
    g_object_unref(source1);

    /*

           node
          /   \
       + +     + +
     source0 source1

     source0 has one sink attached to output 1.
     source1 has one sink attached to output 0.

    */

    ct_test(test, source0 == gegl_node_get_source_node(node, 0)); 
    ct_test(test, 1 == gegl_node_get_source_output(node, 0)); 

    ct_test(test, source1 == gegl_node_get_source_node(node, 1)); 
    ct_test(test, 0 == gegl_node_get_source_output(node, 1)); 

    ct_test(test, 2 == gegl_node_get_num_inputs(node)); 
    ct_test(test, 1 == gegl_node_get_num_outputs(node)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(source0)); 
    ct_test(test, 2 == gegl_node_get_num_outputs(source0)); 

    ct_test(test, 0 == gegl_node_get_num_inputs(source1)); 
    ct_test(test, 2 == gegl_node_get_num_outputs(source1)); 

    ct_test(test, 1 == gegl_node_get_total_num_sinks(source0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(source0, 0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(source0, 1)); 
    ct_test(test, node == gegl_node_get_sink(source0, 1, 0)); 
    ct_test(test, 0 == gegl_node_get_sink_input(source0, 1, 0)); 

    ct_test(test, 1 == gegl_node_get_total_num_sinks(source1)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(source1, 0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(source1, 1)); 
    ct_test(test, node == gegl_node_get_sink(source1, 0, 0)); 
    ct_test(test, 1 == gegl_node_get_sink_input(source1, 0, 0)); 

    g_object_unref(node);
  }
}

static void
test_node_mult_out_g_object_get(Test *test)
{
  /*

    node2 node3   node3 num_inputs = 1
     -  -  -      node2 num_inputs = 2
     |   \/ 
     |   /\  
     |  /  \  
     + +  + +     node0 num_outputs = 2 
    node0 node1   node1 num_outputs = 2 

  */

  {
    gint num_inputs; 
    gint num_outputs; 

    g_object_get(node2, "num_inputs", &num_inputs, NULL);
    ct_test(test, 2 == num_inputs);

    g_object_get(node0, "num_outputs", &num_outputs, NULL);
    ct_test(test, 2 == num_outputs);
  }

  {
    GeglNode * source0 = NULL; 
    GeglNode * source1 = NULL; 
    gint source0_output;
    gint source1_output;

    g_object_get(node2, "source0", &source0, NULL);
    ct_test(test, node0 == source0);

    g_object_get(node2, "source0_output", &source0_output, NULL);
    ct_test(test, 0 == source0_output);

    g_object_get(node2, "source1", &source1, NULL);
    ct_test(test, node1 == source1);

    g_object_get(node2, "source1_output", &source1_output, NULL);
    ct_test(test, 1 == source1_output);

    g_object_unref(source0);
    g_object_unref(source1);
  }
}

static void
test_node_mult_out_g_object_set(Test *test)
{
  {
    /*

          node
          /  \
       + +    + + 
     source0 source1

     source0 has one sink attached to output 1.
     source1 has one sink attached to output 0.

    */
    
    GeglNode *source0 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);
    GeglNode *source1 = g_object_new(GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);
    GeglNode * node = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  

    g_object_set(node, 
                 "source0", source0, 
                 "source1", source1, 
                 NULL);

    ct_test(test, source0 == gegl_node_get_source_node(node, 0)); 
    ct_test(test, source1 == gegl_node_get_source_node(node, 1)); 

    g_object_set(node, 
                 "source0_output", 1, 
                 "source1_output", 0, 
                 NULL);

    ct_test(test, 1 == gegl_node_get_source_output(node, 0)); 
    ct_test(test, 0 == gegl_node_get_source_output(node, 1)); 

    g_object_unref(node);
    g_object_unref(source0);
    g_object_unref(source1);
  }
}

static void
test_node_mult_out_set_source(Test *test)
{
  {
    /*

          n2   n3   <---- n2 has 2 inputs, n3 has 1 input.
         - -   -      
         |  \  /
         |   \/ 
         |   /\  
         |  /  \  
         + +  + +     
          n0   n1    <---- n0, n1 both have 2 outputs.
    */

    GeglNode *n0, *n1, *n2, *n3;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1, NULL);  

    gegl_node_set_source(n2, n0, 0, 0);  
    gegl_node_set_source(n2, n1, 1, 1);
    gegl_node_set_source(n3, n0, 1, 0);

    /*
          n2      Remove n3.
         - -         
         |  \  
         |   \ 
         |    \  
         |     \  
         + +  + +     
          n0   n1 
    */

    g_object_unref(n3);

    ct_test(test, 1 == gegl_node_get_num_sinks(n0, 0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n0, 1)); 
    ct_test(test, 1 == gegl_node_get_total_num_sinks(n0)); 

    ct_test(test, 0 == gegl_node_get_num_sinks(n1, 0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1, 1)); 

    /*
       Remove n2 
    */

    g_object_unref(n2);

    ct_test(test, 0 == gegl_node_get_num_sinks(n0, 0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n0, 1)); 
    ct_test(test, 0 == gegl_node_get_total_num_sinks(n0)); 

    ct_test(test, 0 == gegl_node_get_num_sinks(n1, 0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n1, 1)); 
    ct_test(test, 0 == gegl_node_get_total_num_sinks(n1)); 

    g_object_unref(n0); 
    g_object_unref(n1);

  }

  {
    /*

          n2   n3   <---- n2 has 2 inputs, n3 has 1 input.
         - -   -      
         |  \  /
         |   \/ 
         |   /\  
         |  /  \  
         + +  + +     
          n0   n1    <---- n0, n1 both have 2 outputs.
    */

    GeglNode *n0, *n1, *n2, *n3;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 2, NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_inputs", 1, NULL);  

    gegl_node_set_source(n2, n0, 0, 0);  
    gegl_node_set_source(n2, n1, 1, 1);
    gegl_node_set_source(n3, n0, 1, 0);


    /* 
       Remove both the sinks for n0: 

          n2   n3
         - -   -      
            \   
             \  
              \  
               \  
         + +  + +     
          n0   n1   
    */
    
    gegl_node_set_source_node(n2, NULL, 0); 
    gegl_node_set_source_node(n3, NULL, 0); 


    ct_test(test, NULL == gegl_node_get_source_node(n2, 0)); 
    ct_test(test, n1 == gegl_node_get_source_node(n2, 1)); 
    ct_test(test, NULL == gegl_node_get_source_node(n3, 0)); 

    ct_test(test, 0 == gegl_node_get_num_sinks(n1, 0)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(n1, 1)); 
    ct_test(test, 1 == gegl_node_get_total_num_sinks(n1)); 

    ct_test(test, 0 == gegl_node_get_num_sinks(n0, 0)); 
    ct_test(test, 0 == gegl_node_get_num_sinks(n0, 1)); 
    ct_test(test, 0 == gegl_node_get_total_num_sinks(n0)); 

    g_object_unref(n0);
    g_object_unref(n1);
    g_object_unref(n2);
    g_object_unref(n3);
  }
}


static void
test_node_mult_out_unlink(Test *test)
{
  {
    /*

          n3   n4  
         - -   -      
         |  \  /
         |   \/ 
         |   /\  
         |  /  \  
         + +  + +     
          n1   n2
         - -  
         | 
       + + 
        n0
    */

    GeglNode *n0, *n1, *n2, *n3, *n4;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       "num_inputs", 2, 
                       "source0", n0,
                       "source0_output", 1,
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 2, 
                       "source0", n1,
                       "source0_output", 0,
                       "source1", n2,
                       "source1_output", 1,
                       NULL);  
    n4 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "source0", n1,
                       "source0_output", 1,
                       NULL);  

    gegl_node_unlink(n1);

    /*
       After unlinking n1:

          n3   n4      
         - -   -      
            \   
             \  
              \  
               \  
         + +  + +     
          n1   n2
         - -  
              
       + + 
        n0
    */

    ct_test(test, 0 == gegl_node_get_total_num_sinks(n1)); 
    ct_test(test, NULL == gegl_node_get_source_node(n1, 0)); 
    ct_test(test, NULL == gegl_node_get_source_node(n1, 1)); 

    ct_test(test, 0 == gegl_node_get_total_num_sinks(n0)); 
    ct_test(test, NULL == gegl_node_get_source_node(n3, 0)); 
    ct_test(test, n2 == gegl_node_get_source_node(n3, 1)); 

    ct_test(test, NULL == gegl_node_get_source_node(n4, 0)); 

    g_object_unref(n0);
    g_object_unref(n1);
    g_object_unref(n2);
    g_object_unref(n3);
    g_object_unref(n4);
  }
}

static void
test_node_mult_out_remove_sources(Test *test)
{
  {
    /*

          n3   n4  
         - -   -      
         |  \  /
         |   \/ 
         |   /\  
         |  /  \  
         + +  + +     
          n1   n2
         - -  
         | 
       + + 
        n0
    */

    GeglNode *n0, *n1, *n2, *n3, *n4;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       "num_inputs", 2, 
                       "source0", n0,
                       "source0_output", 1,
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 2, 
                       "source0", n1,
                       "source0_output", 0,
                       "source1", n2,
                       "source1_output", 1,
                       NULL);  
    n4 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "source0", n1,
                       "source0_output", 1,
                       NULL);  

    gegl_node_remove_sources(n3);

    ct_test(test, NULL == gegl_node_get_source_node(n3, 0)); 
    ct_test(test, NULL == gegl_node_get_source_node(n3, 1)); 

    ct_test(test, 1 == gegl_node_get_total_num_sinks(n1)); 
    ct_test(test, n0 == gegl_node_get_source_node(n1, 0)); 

    ct_test(test, 0 == gegl_node_get_total_num_sinks(n2)); 
    ct_test(test, NULL == gegl_node_get_source_node(n3, 0)); 
    ct_test(test, NULL == gegl_node_get_source_node(n3, 1)); 

    ct_test(test, n1 == gegl_node_get_source_node(n4, 0)); 

    g_object_unref(n0);
    g_object_unref(n1);
    g_object_unref(n2);
    g_object_unref(n3);
    g_object_unref(n4);
  }
}

static void
test_node_mult_out_remove_sinks(Test *test)
{
  {
    /*

          n3   n4  
         - -   -      
         |  \  /
         |   \/ 
         |   /\  
         |  /  \  
         + +  + +     
          n1   n2
         - -  
         | 
       + + 
        n0
    */

    GeglNode *n0, *n1, *n2, *n3, *n4;

    n0 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n1 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       "num_inputs", 2, 
                       "source0", n0,
                       "source0_output", 1,
                       NULL);  
    n2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_outputs", 2, 
                       NULL);  
    n3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 2, 
                       "source0", n1,
                       "source0_output", 0,
                       "source1", n2,
                       "source1_output", 1,
                       NULL);  
    n4 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                       "num_inputs", 1, 
                       "source0", n1,
                       "source0_output", 1,
                       NULL);  

    gegl_node_remove_sinks(n1);

    ct_test(test, NULL == gegl_node_get_source_node(n3, 0)); 
    ct_test(test, n2 == gegl_node_get_source_node(n3, 1)); 

    ct_test(test, 0 == gegl_node_get_total_num_sinks(n1)); 
    ct_test(test, n0 == gegl_node_get_source_node(n1, 0)); 

    ct_test(test, 1 == gegl_node_get_total_num_sinks(n2)); 

    ct_test(test, NULL == gegl_node_get_source_node(n4, 0)); 

    g_object_unref(n0);
    g_object_unref(n1);
    g_object_unref(n2);
    g_object_unref(n3);
    g_object_unref(n4);
  }
}

static void
node_mult_out_setup(Test *test)
{

  /*

    node2 node3   <---- node2 has 2 inputs, node3 has 1 input.
     -  -  -      
     |   \/ 
     |   /\  
     |  /  \  
     + +  + +     
    node0 node1    <---- node0, node1 both have 2 outputs.
                   <---- node0 has 2 sinks, node 1 has 1 sink.

  */

  node0 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  
  node1 = g_object_new (GEGL_TYPE_MOCK_NODE, "num_outputs", 2, NULL);  

  node2 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 2,
                        "source0", node0,     /* source0_output defaults to 0 */
                        "source1", node1, 
                        "source1_output", 1, 
                        NULL);  

  node3 = g_object_new (GEGL_TYPE_MOCK_NODE, 
                        "num_inputs", 1,
                        "source0", node0,
                        "source0_output", 1, 
                        NULL);  

}

static void
node_mult_out_teardown(Test *test)
{
  g_object_unref(node0);
  g_object_unref(node1);
  g_object_unref(node2);
  g_object_unref(node3);
}

Test *
create_node_mult_out_test()
{
  Test *t = ct_create("GeglNodeMultOutTest");

  g_assert(ct_addSetUp(t, node_mult_out_setup));
  g_assert(ct_addTearDown(t, node_mult_out_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_node_mult_out_g_object_new));
  g_assert(ct_addTestFun(t, test_node_mult_out_g_object_get));
  g_assert(ct_addTestFun(t, test_node_mult_out_g_object_set));
  g_assert(ct_addTestFun(t, test_node_mult_out_set_source));
  g_assert(ct_addTestFun(t, test_node_mult_out_unlink));
  g_assert(ct_addTestFun(t, test_node_mult_out_remove_sources));
  g_assert(ct_addTestFun(t, test_node_mult_out_remove_sinks));

#endif

  return t;
}
