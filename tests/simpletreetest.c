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
  fade fill2 
      |
     fill1  

          (.55,.7,.85)
          /         \
    (.05,.1,.15)   (.5,.6,.7)
         | 
    (.1,.2,.3)

  */ 

  GeglOp * fill1 = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * fill2 = testutils_fill("RgbFloat",.5,.6,.7,0); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                      "source", fill1,
                                      "multiplier", .5,
                                      NULL); 

  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "source0", fade,
                               "source1", fill2,
                               NULL);  

#if 0
  gegl_op_apply_image(add, GEGL_IMAGE(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .55, .7, .85));  
#endif

  gegl_rect_set(&roi, 1,1,1,1);
  gegl_op_apply_image(add, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 1, 1, .55, .7, .85));  

  g_object_unref(add);
  g_object_unref(fade);

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
  fade   
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

  GeglOp * fill1 = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * print1 = g_object_new(GEGL_TYPE_PRINT,
                                 "source", fill1,
                                 NULL);

  GeglOp * fill2 = testutils_fill("RgbFloat",.5,.6,.7,0); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "source", print1,
                                "multiplier", .5,
                                NULL); 

  GeglOp * print2 = g_object_new(GEGL_TYPE_PRINT,
                                 "source", fade,
                                 NULL);
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "source0", print2,
                               "source1", fill2,
                               NULL);  

  gegl_op_apply_image(add, GEGL_OP(dest), NULL); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .55, .7, .85));  

  g_object_unref(add);
  g_object_unref(fade);
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
    fade1 fade2 
       \    / 
        fill  

            (.25,.5,.75)
            /         \
    (.05,.1,.15)   (.2,.4,.6)
            \        / 
            (.1,.2,.3)

  */ 

  GeglOp * fill = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                  "source", fill,
                                  "multiplier", .5,
                                  NULL); 
  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                  "source", fill,
                                  "multiplier", 2.0,
                                  NULL); 
  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                               "source0", fade1,
                               "source1", fade2,
                               NULL);  

  gegl_op_apply(add); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(add), .25, .5, .75));  

  g_object_unref(add);
  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(fill);
}

static void
test_simple_chain_apply(Test *t)
{
  /* 
      fade2 
        |  
      fade1  
        |
      fill  

    (.025,.05,.075)
         | 
    (.05,.1,.15)
         |
    (.1,.2,.3)

  */ 

  GeglOp * fill = testutils_fill("RgbFloat",.1,.2,.3,0); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                      "source", fill,
                                      "multiplier", .5,
                                      NULL); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                      "source", fade1,
                                      "multiplier", .5,
                                      NULL); 

  gegl_op_apply(fade2); 

  ct_test(t, testutils_check_rgb_float_pixel(GEGL_IMAGE(fade2), .025, .05, .075));  

  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(fill);
}

static void
test_simple_roi_change(Test *t)
{

  /* 
     fade
       |
     fill  
              

     (.05,.1,.15)
        |
     (.1,.2,.3)

  */ 

  GeglRect roi = {0,0,1,1};
  GeglOp * fill = testutils_fill("RgbFloat",.1,.2,.3,0); 
  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                      "source", fill,
                                      "multiplier", .5,
                                      NULL); 

  gegl_op_apply_image(fade, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 0, 0, .05, .1, .15));  
  

#if 0
  /* Change the roi and try it again. */
  gegl_rect_set(&roi, 1,1,1,1); 
  gegl_op_apply_image(fade, GEGL_OP(dest), &roi); 
  ct_test(t, testutils_check_rgb_float_pixel_xy(GEGL_IMAGE(dest), 1, 1, .05, .1, .15));  
#endif


  g_object_unref(fade);
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
