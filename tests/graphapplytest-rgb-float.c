#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define IMAGE_WIDTH 20 
#define IMAGE_HEIGHT 20 

static void
test_graph_apply(Test *t)
{
  /* 
     fade2
       |  
    --------
   | fade1 |
   |   |   |  <----graph 
   | color | 
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

  GeglOp * color = g_object_new (GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "source", color,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", fade1, 
                                 NULL);

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                 "multiplier", .5,
                                 "source", graph,
                                 NULL); 
                        

  gegl_op_apply(fade2); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(fade2), .025, .05, .075));  

  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(color);
}

static void
test_graph_apply_with_source(Test *t)
{

  /* 
    --------
   | fade  |  <--graph
   ---------
       |
     color 
      
     graph
       | 
     color 

    --------------
    |(.05,.1,.15)|
    ------------- 
          |
      (.1,.2,.3)

  */ 

  GeglOp * color = g_object_new (GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "multiplier", .5,
                                NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", fade, 
                                 "source", color,
                                 NULL);

  gegl_op_apply(graph); 

  /* Note: The result is in the fade image data */
  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(fade), .05, .1, .15));  

  g_object_unref(graph);
  g_object_unref(fade);
  g_object_unref(color);
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
     color 

   
     fade2
       |
     graph
       |
     color

    (.025,.05,.075)
          |          
    --------------
    |(.05,.1,.15)|
    ------------- 
          |
     (.1,.2,.3) 

  */ 

  GeglOp * color = g_object_new (GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", fade1, 
                                 "source", color,
                                 NULL);

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                 "source", graph,
                                 "multiplier", .5,
                                 NULL); 

  gegl_op_apply(fade2); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(fade2), .025, .05, .075));  

  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(color);
}

static void
test_graph_apply_with_2_ops_source_and_output(Test *t)
{
  /* 
     fade3
       |  
    --------
   | fade2 | <----graph
   |   |   |
   | fade1 |
   ---------
       |
     color 

   
     fade3
       |
     graph
       |
     color 

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


  GeglOp * color = g_object_new (GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                 "source", fade1,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", fade2, 
                                 "source", color,
                                 NULL);

  GeglOp * fade3 = g_object_new (GEGL_TYPE_FADE,
                                 "source", graph,
                                 "multiplier", .5,
                                 NULL); 

  gegl_op_apply(fade3); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(fade3), .0125, .025, .0375));  

  g_object_unref(fade3);
  g_object_unref(fade2);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(color);
}

static void
test_graph_apply_add_graph_and_fill(Test *t)
{
  /*        
             iadd
            /    \ 
           /      \
      --------   color2 
     | fade   |   
     |   |    |    
     | color1 | 
     ---------  

             iadd
            /    \ 
           /      \
      graph      color2  


           (.45,.6,.75)
            /        \
           /          \         
    --------------   (.4,.5,.6)
    |(.05,.1,.15)|     
    |     |      |  
    |(.1,.2,.3)  | 
    -------------  

  */ 

  GeglOp * color1 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .1, .2, .3, 
                                  NULL); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "source", color1,
                                "multiplier", .5,
                                NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", fade, 
                                 NULL);

  GeglOp * color2 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .4, .5, .6, 
                                  NULL); 

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD, 
                                "source0", graph,
                                "source1", color2,
                                NULL);  
                        
  gegl_op_apply(iadd); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(iadd), .45, .6, .75));  

  g_object_unref(iadd);
  g_object_unref(color2);
  g_object_unref(graph);
  g_object_unref(fade);
  g_object_unref(color1);
}

static void
test_graph_apply_add_graph_and_graph(Test *t)
{
  /*        
             iadd
            /    \ 
           /      \
      --------    -------------
     | fade   |  |    add1     |
     |   |    |  |   /    \    |  
     | color1 |  |color2 color3| 
     ---------   --------------

             iadd
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

  GeglOp * color1 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .1, .2, .3, 
                                  NULL); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "source", color1,
                                "multiplier", .5,
                                NULL); 

  GeglOp * graph1 = g_object_new (GEGL_TYPE_GRAPH,
                                  "root", fade, 
                                  NULL);

  GeglOp * color2 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .4, .5, .6, 
                                  NULL); 

  GeglOp * color3 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .7, .8, .9, 
                                  NULL); 

  GeglOp * iadd1 = g_object_new (GEGL_TYPE_I_ADD,
                                 "source0", color2,
                                 "source1", color3,
                                 NULL); 

  GeglOp * graph2 = g_object_new (GEGL_TYPE_GRAPH,
                                  "root", iadd1, 
                                  NULL);

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD,
                               "source0", graph1,
                               "source1", graph2,
                               NULL); 
                        
  gegl_op_apply(iadd); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(iadd), 1.15, 1.4, 1.65));  

  g_object_unref(iadd);
  g_object_unref(graph2);
  g_object_unref(graph1);
  g_object_unref(fade);
  g_object_unref(color1);
  g_object_unref(iadd1);
  g_object_unref(color2);
  g_object_unref(color3);
}

static void
test_graph_apply_with_2_sources(Test *t)
{
  /*        
             fade1 
              | 
         -------------
        |    iadd   |
        |   /    \  |  
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

  GeglOp * color1 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .1, .2, .3, 
                                  NULL); 

  GeglOp * color2 = g_object_new (GEGL_TYPE_COLOR, 
                                  "pixel-rgb-float", .4, .5, .6, 
                                  NULL); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * fade3 = g_object_new (GEGL_TYPE_FADE,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD,
                                "source0", fade2,
                                "source1", fade3,
                                NULL); 

  GeglOp * graph = g_object_new (GEGL_TYPE_GRAPH,
                                 "root", iadd, 
                                 "source0", color1,
                                 "source1", color2,
                                 NULL);

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "source", graph,
                                 "multiplier", .5,
                                 NULL); 

  gegl_op_apply(fade1); 

  ct_test(t, testutils_check_rgb_float(GEGL_IMAGE(fade1), .125, .175, .225));  

  g_object_unref(iadd);
  g_object_unref(graph);
  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(fade3);
  g_object_unref(color1);
  g_object_unref(color2);
}

static void
graph_apply_test_setup(Test *test)
{
}

static void
graph_apply_test_teardown(Test *test)
{
}

Test *
create_graph_apply_test_rgb_float()
{
  Test* t = ct_create("GeglGraphApplyTestRgbFloat");

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
