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
     fade2
       |  
    --------
   | fade1 |
   |   |    |  <----graph 
   |  fill  | 
   ---------

     fade2
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

  GeglOp * fill = testutils_fill("RgbFloat", .1,.2,.3, 0); 
  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                  "source", fill,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                "root", fade1, 
                                NULL);

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  "source", graph,
                                  NULL); 
                        

  gegl_op_apply_image(fade2, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(fill);
}

static void
test_graph_apply_with_source(Test *t)
{

  /* 
    --------
   | fade |  <--graph
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

  GeglOp * fill = testutils_fill("RgbFloat", .1,.2,.3, 0); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", fade, 
                                 "source", fill,
                                 NULL);

  gegl_op_apply_image(graph, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .05, .1, .15));  

  g_object_unref(graph);
  g_object_unref(fade);
  g_object_unref(fill);
}

static void
test_graph_apply_with_source_and_output(Test *t)
{
  /* 
     fade2
       |  
    --------
   | fade1 | <----graph
   ---------
       |
      fill

   
     fade2
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

  GeglOp * fill = testutils_fill("RgbFloat", .1,.2,.3, 0); 
  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", fade1, 
                                 "source", fill,
                                 NULL);

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_op_apply_image(fade2, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(fill);
}

static void
test_graph_apply_with_2_ops_source_and_output(Test *t)
{
  /* 
     fade3
       |  
    --------
   | fade2 | <----graph
   |   |    |
   | fade1 |
   ---------
       |
      fill

   
     fade3
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

  GeglOp * fill = testutils_fill("RgbFloat", .1,.2,.3, 0); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  NULL); 
  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "source", fade1,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                "root", fade2, 
                                "source", fill,
                                NULL);

  GeglOp * fade3 = g_object_new (GEGL_TYPE_FADE,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_op_apply_image(fade3, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .0125, .025, .0375));  

  g_object_unref(fade3);
  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
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
     | fade  |   
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

  GeglOp * fill1 = testutils_fill("RgbFloat", .1,.2,.3, 0); 
  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                 "source", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                "root", fade, 
                                NULL);

  GeglOp * fill2 = testutils_fill("RgbFloat" , .4,.5,.6, 0); 
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "source0", graph,
                               "source1", fill2,
                               NULL);  
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .45, .6, .75));  

  g_object_unref(add);
  g_object_unref(fill2);
  g_object_unref(graph);
  g_object_unref(fade);
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
     | fade  |  |    add1     |
     |   |    |  |   /    \    |  
     | fill1  |  | fill2 fill3 | 
     ---------   --------------

              add
            /    \ 
           /      \
      graph1    graph2  


           (1.15,1.4,1.65)
            /        \
           /          \         
    --------------   ----------------------
    |(.05,.1,.15)|   |    (1.1,1.3,1.5)    |
    |     |      |   |     /        \      |
    |(.1,.2,.3)  |   |(.4,.5,.6) (.7,.8,.9)|
    -------------     --------------------- 

  */ 

  GeglOp * fill1 = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                 "source", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * graph1 = g_object_new(GEGL_TYPE_GRAPH,
                                  "root", fade, 
                                  NULL);

  GeglOp * fill2 = testutils_fill("RgbFloat",.4,.5,.6,0); 
  GeglOp * fill3 = testutils_fill("RgbFloat" ,.7,.8,.9,0); 
  GeglOp * add1 = g_object_new (GEGL_TYPE_ADD,
                                "source0", fill2,
                                "source1", fill3,
                                NULL); 
  GeglOp * graph2 = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", add1, 
                                 NULL);

  GeglOp * add = g_object_new (GEGL_TYPE_ADD,
                               "source0", graph1,
                               "source1", graph2,
                               NULL); 
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), 1.15, 1.4, 1.65));  

  g_object_unref(add);
  g_object_unref(graph2);
  g_object_unref(graph1);
  g_object_unref(fade);
  g_object_unref(fill1);
  g_object_unref(add1);
  g_object_unref(fill2);
  g_object_unref(fill3);
}

static void
test_graph_apply_with_2_sources(Test *t)
{
  /*        
             fade1 
              | 
         -------------
        |    add      |
        |   /    \    |  
        |fade2 fade3| 
        --------------
           |      |
          fill1  fill2

           fade1 
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

  GeglOp * fill1 = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * fill2 = testutils_fill("RgbFloat",.4,.5,.6,0); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * fade3 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * add = g_object_new (GEGL_TYPE_ADD,
                               "source0", fade2,
                               "source1", fade3,
                               NULL); 

  GeglOp * graph = g_object_new(GEGL_TYPE_GRAPH,
                                 "root", add, 
                                 "source0", fill1,
                                 "source1", fill2,
                                 NULL);

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                  "source", graph,
                                  "multiplier", .5,
                                  NULL); 

  gegl_op_apply(fade1); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(fade1), .125, .175, .225));  

  g_object_unref(add);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(fade3);
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
