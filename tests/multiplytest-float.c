#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

#define R0 .1 
#define G0 .2
#define B0 .3 
#define A0 .6 


#define R1 .4 
#define G1 .5
#define B1 .6  
#define A1 .4  

static void
test_multiply_g_object_new(Test *test)
{
  {
    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, NULL);  

    ct_test(test, GEGL_IS_MULTIPLY(multiply));
    ct_test(test, g_type_parent(GEGL_TYPE_MULTIPLY) == GEGL_TYPE_BLEND);
    ct_test(test, !strcmp("GeglMultiply", g_type_name(GEGL_TYPE_MULTIPLY)));

    g_object_unref(multiply);
  }
}

static void
test_multiply_apply_rgb_rgb(Test *test)
{
  GeglOp * background = g_object_new(GEGL_TYPE_COLOR, 
                                     "width", IMAGE_OP_WIDTH, 
                                     "height", IMAGE_OP_HEIGHT, 
                                     "pixel-rgb-float", R0, G0, B0, 
                                     NULL); 

  GeglOp * foreground = g_object_new(GEGL_TYPE_COLOR, 
                                     "width", IMAGE_OP_WIDTH, 
                                     "height", IMAGE_OP_HEIGHT, 
                                     "pixel-rgb-float", R1, G1, B1, 
                                     NULL); 
  {
    /* 
       multiply = source0 * source1 
       (.04,.10,.18) = (.1,.2,.3) * (.4,.5,.6)
    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "background", background,
                                      "foreground", foreground,
                                      NULL);  

    gegl_op_apply(multiply); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(multiply), 
                                        .04, .10, .18));
    g_object_unref(multiply);
  }

  g_object_unref(background);
  g_object_unref(foreground);
}

static void
test_multiply_apply_rgba_rgba(Test *test)
{
  GeglOp * background = g_object_new(GEGL_TYPE_COLOR, 
                                     "width", IMAGE_OP_WIDTH, 
                                     "height", IMAGE_OP_HEIGHT, 
                                     "pixel-rgba-float", R0, G0, B0, A0, 
                                     NULL); 

  GeglOp * foreground = g_object_new(GEGL_TYPE_COLOR, 
                                     "width", IMAGE_OP_WIDTH, 
                                     "height", IMAGE_OP_HEIGHT, 
                                     "pixel-rgba-float", R1, G1, B1, A1, 
                                     NULL); 
  {
    /* 
       (f, fa) = (.4,.5,.6,.4)
       (b, ba) = (.1,.2,.3,.6)

       d = (1-fa)*b + (1-ba)*f + f*b;
       da = fa + ba - fa*ba;

       1 - fa = .6;
       1 - ba = .4;

       (d, da) = (.6*b + .4*f + f*b, .4 + .6 - .4*.6)
               = (.6*.1 + .4*.4 + .4*.1, 
                  .6*.2 + .4*.5 + .5*.2, 
                  .6*.3 + .4*.6 + .6*.3,
                  .76)
               = (.26, .42, .6, .76) 

    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "background", background,
                                      "foreground", foreground,
                                      NULL);  

    gegl_op_apply(multiply); 

    ct_test(test, testutils_check_pixel_rgba_float(GEGL_IMAGE_OP(multiply),
                                        .26, .42, .6, .76));
    g_object_unref(multiply);
  }

  g_object_unref(background);
  g_object_unref(foreground);
}

static void
multiply_test_setup(Test *test)
{
}

static void
multiply_test_teardown(Test *test)
{
}

Test *
create_multiply_test_float()
{
  Test* t = ct_create("GeglMultiplyTestFloat");

  g_assert(ct_addSetUp(t, multiply_test_setup));
  g_assert(ct_addTearDown(t, multiply_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_multiply_g_object_new));
  g_assert(ct_addTestFun(t, test_multiply_apply_rgb_rgb));
  g_assert(ct_addTestFun(t, test_multiply_apply_rgba_rgba));
#endif

  return t; 
}
