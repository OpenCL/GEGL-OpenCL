#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

static void
test_simple_tree_apply(Test *t)
{
  /* 
        iadd 
        /   \ 
     fade  color2 
      |
     color1  

          (.55,.7,.85)
          /         \
    (.05,.1,.15)   (.5,.6,.7)
         | 
    (.1,.2,.3)

  */ 

  GeglOp * color1 = g_object_new(GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * color2 = g_object_new(GEGL_TYPE_COLOR, 
                                 "pixel-rgb-float", .5, .6, .7, 
                                 NULL); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "source", color1,
                                "multiplier", .5,
                                NULL); 

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD, 
                                "source-0", fade,
                                "source-1", color2,
                                NULL);  

  gegl_op_apply(iadd); 

  ct_test(t, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(iadd), .55, .7, .85));  

  g_object_unref(iadd);
  g_object_unref(fade);
  g_object_unref(color1);
  g_object_unref(color2);
}

static void
test_simple_tree_apply_with_prints(Test *t)
{


  /* 
        iadd
        /   \ 
     print2 color2 
      |
     fade   
      |
     print1
      |
     color1  

          (.55,.7,.85)
          /         \
      print2      (.5,.6,.7)
         | 
    (.05,.1,.15)
         | 
      print1
         |
    (.1,.2,.3)

  */ 

  GeglOp * color1 = g_object_new(GEGL_TYPE_COLOR, 
                                 "width", IMAGE_OP_WIDTH,
                                 "height", IMAGE_OP_HEIGHT,
                                 "pixel-rgb-float", .1, .2, .3, 
                                 NULL); 

  GeglOp * print1 = g_object_new(GEGL_TYPE_PRINT,
                                 "source", color1,
                                 NULL);

  GeglOp * color2 = g_object_new(GEGL_TYPE_COLOR, 
                                 "width", IMAGE_OP_WIDTH,
                                 "height", IMAGE_OP_HEIGHT,
                                 "pixel-rgb-float", .5, .6, .7, 
                                 NULL); 

  GeglOp * fade = g_object_new (GEGL_TYPE_FADE,
                                "source", print1,
                                "multiplier", .5,
                                NULL); 

  GeglOp * print2 = g_object_new(GEGL_TYPE_PRINT,
                                 "source", fade,
                                 NULL);

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD, 
                                "source-0", print2,
                                "source-1", color2,
                                NULL);  

  gegl_op_apply(iadd); 

  ct_test(t, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(iadd), .55, .7, .85));  

  g_object_unref(iadd);
  g_object_unref(fade);
  g_object_unref(color1);
  g_object_unref(color2);
  g_object_unref(print1);
  g_object_unref(print2);
}

static void
test_simple_diamond_apply(Test *t)
{

  /* 
        iadd 
        /   \ 
    fade1 fade2 
       \    / 
        color 

            (.25,.5,.75)
            /         \
    (.05,.1,.15)   (.2,.4,.6)
            \        / 
            (.1,.2,.3)

  */ 

  GeglOp * color = g_object_new(GEGL_TYPE_COLOR, 
                                "width", IMAGE_OP_WIDTH,
                                "height", IMAGE_OP_HEIGHT,
                                "pixel-rgb-float", .1, .2, .3, 
                                NULL); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "source", color,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                 "source", color,
                                 "multiplier", 2.0,
                                 NULL); 

  GeglOp * iadd = g_object_new (GEGL_TYPE_I_ADD, 
                                "source-0", fade1,
                                "source-1", fade2,
                                NULL);  

  gegl_op_apply(iadd); 

  ct_test(t, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(iadd), .25, .5, .75));  

  g_object_unref(iadd);
  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(color);
}

static void
test_simple_chain_apply(Test *t)
{
  /* 
      fade2 
        |  
      fade1  
        |
      color  

    (.025,.05,.075)
         | 
    (.05,.1,.15)
         |
    (.1,.2,.3)

  */ 

  GeglOp * color = g_object_new(GEGL_TYPE_COLOR, 
                                "width", IMAGE_OP_WIDTH,
                                "height", IMAGE_OP_HEIGHT,
                                "pixel-rgb-float", .1, .2, .3, 
                                NULL); 

  GeglOp * fade1 = g_object_new (GEGL_TYPE_FADE,
                                 "source", color,
                                 "multiplier", .5,
                                 NULL); 

  GeglOp * fade2 = g_object_new (GEGL_TYPE_FADE,
                                 "source", fade1,
                                 "multiplier", .5,
                                 NULL); 

  gegl_op_apply(fade2); 

  ct_test(t, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(fade2), .025, .05, .075));  

  g_object_unref(fade1);
  g_object_unref(fade2);
  g_object_unref(color);
}

static void
simple_tree_test_setup(Test *test)
{
}

static void
simple_tree_test_teardown(Test *test)
{
}

Test *
create_simple_tree_test_float()
{
  Test* t = ct_create("GeglSimpleTreeTestFloat");

  g_assert(ct_addSetUp(t, simple_tree_test_setup));
  g_assert(ct_addTearDown(t, simple_tree_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_simple_tree_apply));
  g_assert(ct_addTestFun(t, test_simple_tree_apply_with_prints));
  g_assert(ct_addTestFun(t, test_simple_chain_apply));
  g_assert(ct_addTestFun(t, test_simple_diamond_apply));
#endif

  return t; 
}
