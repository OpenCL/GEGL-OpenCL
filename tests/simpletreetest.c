#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 5 
#define SAMPLED_IMAGE_HEIGHT 5 
GeglSampledImage *dest;

static void
test_simpletree_apply(Test *t)
{
  GeglRect roi = {0,0,1,1};

  /* 
    Evaluate and put result in dest: 

         add  -> dest 
        /   \ 
  const_mult fill2 
      |
     fill1  

          (.55,.7,.85)
          /         \
    (.05,.1,.15)   (.5,.6,.7)
         | 
    (.1,.2,.3)

  */ 

  GeglOp * fill1 = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * fill2 = testutils_rgb_fill(.5,.6,.7); 

  GeglOp * const_mult = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", fill1,
                                      "multiplier", .5,
                                      NULL); 

  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "input0", const_mult,
                               "input1", fill2,
                               NULL);  

#if 0
  gegl_op_apply_image(add, GEGL_IMAGE(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .55, .7, .85));  
#endif

  gegl_rect_set(&roi, 1,1,1,1);
  gegl_op_apply_image(add, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 1, 1, .55, .7, .85));  

  g_object_unref(add);
  g_object_unref(const_mult);

  g_object_unref(fill1);
  g_object_unref(fill2);
}

static void
test_simpletree_apply2(Test *t)
{

  /* 
    Evaluate and put result in dest: 

         add  -> dest 
        /   \ 
     print  fill2 
      |
  const_mult   
      |
     print
      |
     fill1  

          (.55,.7,.85)
          /         \
    (.05,.1,.15)   (.5,.6,.7)
         | 
    (.1,.2,.3)

  */ 

  GeglOp * fill1 = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * print1 = g_object_new(GEGL_TYPE_PRINT,
                                 "input", fill1,
                                 NULL);

  GeglOp * fill2 = testutils_rgb_fill(.5,.6,.7); 

  GeglOp * const_mult = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", print1,
                                      "multiplier", .5,
                                      NULL); 

  GeglOp * print2 = g_object_new(GEGL_TYPE_PRINT,
                                 "input", const_mult,
                                 NULL);
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "input0", print2,
                               "input1", fill2,
                               NULL);  

  gegl_op_apply_image(add, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .55, .7, .85));  

  g_object_unref(add);
  g_object_unref(const_mult);
  g_object_unref(fill1);
  g_object_unref(fill2);
  g_object_unref(print1);
  g_object_unref(print2);
}

static void
test_simple_diamond_apply(Test *t)
{

  /* 
         add 
        /   \ 
    cmult1 cmult2 
       \    / 
        fill  

            (.25,.5,.75)
            /         \
    (.05,.1,.15)   (.2,.4,.6)
            \        / 
            (.1,.2,.3)

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", fill,
                                      "multiplier", .5,
                                      NULL); 
  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", fill,
                                      "multiplier", 2.0,
                                      NULL); 
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "input0", cmult1,
                               "input1", cmult2,
                               NULL);  

  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .25, .5, .75));  

  g_object_unref(add);
  g_object_unref(cmult1);
  g_object_unref(cmult2);
  g_object_unref(fill);
}

static void
test_simple_chain_apply(Test *t)
{
  /* 
      cmult2 
        |  
      cmult1  
        |
      fill  

    (.025,.05,.075)
         | 
    (.05,.1,.15)
         |
    (.1,.2,.3)

  */ 

  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 

  GeglOp * cmult1 = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", fill,
                                      "multiplier", .5,
                                      NULL); 

  GeglOp * cmult2 = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", cmult1,
                                      "multiplier", .5,
                                      NULL); 

  gegl_op_apply(cmult2); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(cmult2), .025, .05, .075));  

  g_object_unref(cmult1);
  g_object_unref(cmult2);
  g_object_unref(fill);
}

static void
test_simple_roi_change(Test *t)
{

  /* 
     cmult
       |
     fill  
              

     (.05,.1,.15)
        |
     (.1,.2,.3)

  */ 

  GeglRect roi = {0,0,1,1};
  GeglOp * fill = testutils_rgb_fill(.1,.2,.3); 
  GeglOp * cmult = g_object_new (GEGL_TYPE_CONST_MULT,
                                      "input", fill,
                                      "multiplier", .5,
                                      NULL); 

  gegl_op_apply_image(cmult, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 0, 0, .05, .1, .15));  
  

#if 0
  /* Change the roi and try it again. */
  gegl_rect_set(&roi, 1,1,1,1); 
  gegl_op_apply_image(cmult, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 1, 1, .05, .1, .15));  
#endif


  g_object_unref(cmult);
  g_object_unref(fill);
}


static void
simpletree_test_setup(Test *test)
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
simpletree_test_teardown(Test *test)
{
  g_object_unref(dest);
}

Test *
create_simpletree_test()
{
  Test* t = ct_create("GeglSimpleTreeTest");

  g_assert(ct_addSetUp(t, simpletree_test_setup));
  g_assert(ct_addTearDown(t, simpletree_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_simpletree_apply));
  g_assert(ct_addTestFun(t, test_simple_chain_apply));
  g_assert(ct_addTestFun(t, test_simple_diamond_apply));
  g_assert(ct_addTestFun(t, test_simple_roi_change));
  g_assert(ct_addTestFun(t, test_simpletree_apply2));
#endif

  return t; 
}
