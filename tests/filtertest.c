#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 20 
#define SAMPLED_IMAGE_HEIGHT 20 
GeglSampledImage *dest;

static void
test_filter_apply(Test *t)
{
  /* 
     cmult1
       |  
    --------
   | cmult2 |
   |   |    |  <----filter 
   |  fill  | 
   ---------

     cmult1
       |
     filter

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
                                 "input", fill,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * filter = g_object_new(GEGL_TYPE_FILTER,
                                 "root", cmult1, 
                                 NULL);

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                 "input", filter,
                                 "multiplier", .5,
                                 NULL); 
                        

  gegl_op_apply_image(cmult2, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(cmult2);
  g_object_unref(filter);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_filter_apply_with_input(Test *t)
{
  /* 
    --------
   | cmult |  <--filter
   ---------
       |
      fill
      
     filter
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

  GeglOp * filter = g_object_new(GEGL_TYPE_FILTER,
                                 "root", cmult, 
                                 NULL);

  gegl_node_set_nth_input(GEGL_NODE(filter), GEGL_NODE(fill), 0);
  gegl_op_apply_image(filter, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .05, .1, .15));  

  g_object_unref(filter);
  g_object_unref(cmult);
  g_object_unref(fill);
}

static void
test_filter_apply_with_input_and_output(Test *t)
{
  /* 
     cmult2
       |  
    --------
   | cmult1 | <----filter
   ---------
       |
      fill

   
     cmult2
       |
     filter
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

  GeglOp * filter = g_object_new(GEGL_TYPE_FILTER,
                                 "root", cmult1, 
                                 NULL);

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "input", filter,
                                  "multiplier", .5,
                                  NULL); 
  gegl_node_set_nth_input(GEGL_NODE(filter), GEGL_NODE(fill), 0);

  gegl_op_apply_image(cmult2, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .025, .05, .075));  

  g_object_unref(cmult2);
  g_object_unref(filter);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_filter_apply_with_2_ops_input_and_output(Test *t)
{
  /* 
     cmult3
       |  
    --------
   | cmult2 | <----filter
   |   |    |
   | cmult1 |
   ---------
       |
      fill

   
     cmult3
       |
     filter
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
                                  "input", cmult1,
                                  "multiplier", .5,
                                  NULL); 

  GeglOp * filter = g_object_new(GEGL_TYPE_FILTER,
                                 "root", cmult2, 
                                 NULL);

  GeglOp * cmult3 = g_object_new (GEGL_TYPE_CONST_MULT,
                                  "input", filter,
                                  "multiplier", .5,
                                  NULL); 

  gegl_node_set_nth_input(GEGL_NODE(filter), GEGL_NODE(fill), 0);
  gegl_op_apply_image(cmult3, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .0125, .025, .0375));  

  g_object_unref(cmult3);
  g_object_unref(cmult2);
  g_object_unref(filter);
  g_object_unref(cmult1);
  g_object_unref(fill);
}

static void
test_filter_apply_add_filter_and_fill(Test *t)
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
      filter     fill2  


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
                                 "input", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * filter = g_object_new(GEGL_TYPE_FILTER,
                                  "root", cmult, 
                                  NULL);

  GeglOp * fill2 = testutils_rgb_fill(.4,.5,.6); 
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                              "input0", filter,
                              "input1", fill2,
                              NULL);  
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .45, .6, .75));  

  g_object_unref(add);
  g_object_unref(fill2);
  g_object_unref(filter);
  g_object_unref(cmult);
  g_object_unref(fill1);
}

static void
test_filter_apply_add_filter_and_filter(Test *t)
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
      filter1    filter2  


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
                                 "input", fill1,
                                 "multiplier", .5,
                                 NULL); 
  GeglOp * filter1 = g_object_new(GEGL_TYPE_FILTER,
                                  "root", cmult, 
                                  NULL);

  GeglOp * fill2 = testutils_rgb_fill(.4,.5,.6); 
  GeglOp * fill3 = testutils_rgb_fill(.7,.8,.9); 
  GeglOp * mult = g_object_new (GEGL_TYPE_MULT,
                                "input0", fill2,
                                "input1", fill3,
                                NULL); 
  GeglOp * filter2 = g_object_new(GEGL_TYPE_FILTER,
                                  "root", mult, 
                                  NULL);

  GeglOp * add = g_object_new (GEGL_TYPE_ADD,
                                "input0", filter1,
                                "input1", filter2,
                                NULL); 
                        
  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .33, .5, .69));  

  g_object_unref(add);
  g_object_unref(filter2);
  g_object_unref(filter1);
  g_object_unref(cmult);
  g_object_unref(fill1);
  g_object_unref(mult);
  g_object_unref(fill2);
  g_object_unref(fill3);
}

static void
filter_test_setup(Test *test)
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
filter_test_teardown(Test *test)
{
  g_object_unref(dest);
}

Test *
create_filter_test()
{
  Test* t = ct_create("GeglFilterTest");

  g_assert(ct_addSetUp(t, filter_test_setup));
  g_assert(ct_addTearDown(t, filter_test_teardown));
  g_assert(ct_addTestFun(t, test_filter_apply));
  g_assert(ct_addTestFun(t, test_filter_apply_with_input));
  g_assert(ct_addTestFun(t, test_filter_apply_with_input_and_output));
  g_assert(ct_addTestFun(t, test_filter_apply_with_2_ops_input_and_output));
  g_assert(ct_addTestFun(t, test_filter_apply_add_filter_and_fill));
  g_assert(ct_addTestFun(t, test_filter_apply_add_filter_and_filter));
  return t; 
}
