#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 20 
#define SAMPLED_IMAGE_HEIGHT 20 
GeglSampledImage *dest;

static void
test_graph_apply(Test *t)
{
  /* 
     cmult1
       |  
    --------
   | cmult2 |
   |   |    |  <----graph 
   |  fill  | 
   ---------

     cmult1
       |
     graph

    (.025,.05,.075)
          |          
    --------------
    |(.05,.1,.15)|
    |     |      |
    |(.1,.2,.3)  |
    ------------- 

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", fill,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                "root", cmult1, 
                                NULL);

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 
                        

  gegl_op_apply_image(cmult2, GEGL_OP(dest), NULL); 
  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(cmult2);
  g_object_unref(graph);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_graph_apply_with_source(Test *t)
{

  /* 
    --------
   | cmult |  <--graph
   ---------
       |
      fill
      
     graph
       | 
      fill

    --------------
    |(.05,.1,.15)|
    ------------- 
          |
      (.1,.2,.3)

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 

  GeglOp * cmult = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", cmult, 
                                 NULL);

  gegl_node_set_source_node(GEGL_NODE(graph), GEGL_NODE(fill), 0);

  LOG_DEBUG("test_graph_apply_with_source", "calling apply");
  gegl_op_apply_image(graph, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .05, .1, .15));  

  g_object_unref(graph);
  g_object_unref(cmult);
  g_object_unref(fill);
}

static void
test_graph_apply_with_source_and_output(Test *t)
{
  /* 
     cmult2
       |  
    --------
   | cmult1 | <----graph
   ---------
       |
      fill

   
     cmult2
       |
     graph
       |
      fill

    (.025,.05,.075)
          |          
    --------------
    |(.05,.1,.15)|
    ------------- 
          |
     (.1,.2,.3) 

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", cmult1, 
                                 NULL);

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_node_set_source_node(GEGL_NODE(graph), GEGL_NODE(fill), 0);

  gegl_op_apply_image(cmult2, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(cmult2);
  g_object_unref(graph);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_graph_apply_with_2_ops_source_and_output(Test *t)
{
  /* 
     cmult3
       |  
    --------
   | cmult2 | <----graph
   |   |    |
   | cmult1 |
   ---------
       |
      fill

   
     cmult3
       |
     graph
       |
      fill

    (.0125,.025,.0375)
          |          
    ----------------
    |(.025,.05,.075)|
    |     |         |
    |(.05,.1,.15)   |  
    ---------------- 
          |
     (.1,.2,.3) 

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 

  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", cmult1,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", cmult2, 
                                 NULL);

  GeglOp * cmult3 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_node_set_source_node(GEGL_NODE(graph), GEGL_NODE(fill), 0);
  gegl_op_apply_image(cmult3, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .0125, .025, .0375));  

  g_object_unref(cmult3);
  g_object_unref(cmult2);
  g_object_unref(graph);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_graph_apply_add_graph_and_fill(Test *t)
{
  /*        
              add
            /    \ 
           /      \
      --------   fill2 
     | cmult  |   
     |   |    |    
     | fill1  | 
     ---------  

             add
            /    \ 
           /      \
      graph     fill2  


           (.45,.6,.75)
            /        \
           /          \         
    --------------   (.4,.5,.6)
    |(.05,.1,.15)|     
    |     |      |  
    |(.1,.2,.3)  | 
    -------------  

  */ 

  GeglOp * fill1 = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "source", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                  "root", cmult, 
                                  NULL);

  GeglOp * fill2 = testutils_rgb_fill(.4,.5,.6); 
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                              "source0", graph,
                              "source1", fill2,
                              NULL);  
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .45, .6, .75));  

  g_object_unref(add);
  g_object_unref(fill2);
  g_object_unref(graph);
  g_object_unref(cmult);
  g_object_unref(fill1);
}

static void
test_graph_apply_add_graph_and_graph(Test *t)
{
  /*        
              add
            /    \ 
           /      \
      --------    -------------
     | cmult  |  |    mult     |
     |   |    |  |   /    \    |  
     | fill1  |  | fill2 fill3 | 
     ---------   --------------

              add
            /    \ 
           /      \
      graph1    graph2  


           (.33,.5,.69)
            /        \
           /          \         
    --------------   ----------------------
    |(.05,.1,.15)|   |    (.28,.4,.54)     |
    |     |      |   |     /        \      |
    |(.1,.2,.3)  |   |(.4,.5,.6) (.7,.8,.9)|
    -------------     --------------------- 

  */ 

  GeglOp * fill1 = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "source", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * graph1 = g_object_new(GEGL_TYPE_GRAPH,
                                  "root", cmult, 
                                  NULL);

  GeglOp * fill2 = testutils_rgb_fill(.4,.5,.6); 
  GeglOp * fill3 = testutils_rgb_fill(.7,.8,.9); 
  GeglOp * mult = g_object_new (GEGL_TYPE_MULT,
                                "source0", fill2,
                                "source1", fill3,
                                NULL); 
  GeglOp * graph2 = g_object_new(GEGL_TYPE_GRAPH,
                                  "root", mult, 
                                  NULL);

  GeglOp * add = g_object_new (GEGL_TYPE_ADD,
                                "source0", graph1,
                                "source1", graph2,
                                NULL); 
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .33, .5, .69));  

  g_object_unref(add);
  g_object_unref(graph2);
  g_object_unref(graph1);
  g_object_unref(cmult);
  g_object_unref(fill1);
  g_object_unref(mult);
  g_object_unref(fill2);
  g_object_unref(fill3);
}

static void
test_graph_apply_with_2_sources(Test *t)
{
  /*        
             cmult1 
              | 
         -------------
        |    add      |
        |   /    \    |  
        |cmult2 cmult3| 
        --------------
           |      |
          fill1  fill2

           cmult1 
             |
           graph  
           /     \ 
        fill1   fill2 



          (.125,.175,.225)
              |
     ----------------------------
     |    (.25,.35,.45)         | 
     |    /         \           | 
     |(.05,.1,.15)(.2,.25,.3)   |
      -------------------------- 
          |          |
      (.1,.2,.3)  (.4,.5, .6)

  */ 

  GeglOp * fill1 = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * fill2 = testutils_rgb_fill(.4,.5,.6); 

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * cmult3 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * add = g_object_new (GEGL_TYPE_ADD,
                               "source0", cmult2,
                               "source1", cmult3,
                               NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", add, 
                                 NULL);

  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_node_set_source_node(GEGL_NODE(graph), GEGL_NODE(fill1), 0);
  gegl_node_set_source_node(GEGL_NODE(graph), GEGL_NODE(fill2), 1);

  gegl_op_apply(cmult1); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(cmult1), .125, .175, .225));  

  g_object_unref(add);
  g_object_unref(graph);
  g_object_unref(cmult1);
  g_object_unref(cmult2);
  g_object_unref(cmult3);
  g_object_unref(fill1);
  g_object_unref(fill2);
}

static void
graph_apply_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

  g_object_unref(rgb_float);
}

static void
graph_apply_test_teardown(Test *test)
{
  g_object_unref(dest);
}

Test *
create_graph_apply_test()
{
  Test* t = ct_create("GeglGraphApplyTest");

  g_assert(ct_addSetUp(t, graph_apply_test_setup));
  g_assert(ct_addTearDown(t, graph_apply_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_graph_apply));
  g_assert(ct_addTestFun(t, test_graph_apply_with_source));
  g_assert(ct_addTestFun(t, test_graph_apply_with_source_and_output));
  g_assert(ct_addTestFun(t, test_graph_apply_with_2_ops_source_and_output));
  g_assert(ct_addTestFun(t, test_graph_apply_add_graph_and_fill));
  g_assert(ct_addTestFun(t, test_graph_apply_add_graph_and_graph));
  g_assert(ct_addTestFun(t, test_graph_apply_with_2_sources));
#endif

  return t; 
}
