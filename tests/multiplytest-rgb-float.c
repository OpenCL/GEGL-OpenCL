#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_WIDTH 1 
#define IMAGE_HEIGHT 1 

#define R0 .1 
#define G0 .2
#define B0 .3 

#define R1 .4 
#define G1 .5
#define B1 .6  

static GeglOp *source0, *source1;

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
test_multiply_properties(Test *test)
{
  {
    GeglOp * multiply =  g_object_new (GEGL_TYPE_MULTIPLY, NULL);  

    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(multiply), 0));
    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(multiply), 1));
    ct_test(test, 2 == gegl_node_get_num_inputs(GEGL_NODE(multiply)));
    ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(multiply)));

    g_object_unref(multiply);
  }

  {
    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0,
                                      "source1", source1,
                                      NULL);  

    ct_test(test, source0 == (GeglOp*)gegl_node_get_source(GEGL_NODE(multiply), 0));
    ct_test(test, source1 == (GeglOp*)gegl_node_get_source(GEGL_NODE(multiply), 1));

    g_object_unref(multiply);
  }
}


static void
test_multiply_apply(Test *test)
{
  {
    /* 
       multiply = source0 * source1 
       (.04,.10,.18) = (.1,.2,.3) * (.4,.5,.6)
    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0,
                                      "source1", source1,
                                      NULL);  

    gegl_op_apply(multiply); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(multiply), .04, .10, .18));
    g_object_unref(multiply);
  }
}

#if 0
static void
test_multiply_apply_rgba_float(Test *test)
{
  {
    /* 
       (f, fa) = (.4,.5,.6,.4)
       (b, ba) = (.1,.2,.3,.6)

       d = (1-fa)*b + (1-ba)*f + f*b;
       da = fa + ba - fa*ba;

       1 - fa = .6;
       1 - ba = .4;

       (d, da) = (.6*b + .4*f + f*b, .4 + .6 - .4 * .6)
               = (.26, .42, .6, .76) 

    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0_rgba,
                                      "source1", source1_rgba,
                                      NULL);  

    gegl_op_apply(multiply); 

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(multiply), "rgbafloat", 
                                        .26, .42, .6, .76));
    g_object_unref(multiply);
  }
}
#endif

static void
multiply_test_setup(Test *test)
{
  source0 = g_object_new(GEGL_TYPE_COLOR, 
                         "width", IMAGE_WIDTH, 
                         "height", IMAGE_HEIGHT, 
                         "pixel-rgb-float", R0, G0, B0, 
                         NULL); 

  source1 = g_object_new(GEGL_TYPE_COLOR, 
                         "width", IMAGE_WIDTH, 
                         "height", IMAGE_HEIGHT, 
                         "pixel-rgb-float", R1, G1, B1, 
                         NULL); 
}

static void
multiply_test_teardown(Test *test)
{
  g_object_unref(source0);
  g_object_unref(source1);
}

Test *
create_multiply_test_pixel_rgb_float()
{
  Test* t = ct_create("GeglMultiplyTestPixelRgbFloat");

  g_assert(ct_addSetUp(t, multiply_test_setup));
  g_assert(ct_addTearDown(t, multiply_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_multiply_g_object_new));
  g_assert(ct_addTestFun(t, test_multiply_properties));
  g_assert(ct_addTestFun(t, test_multiply_apply));
#endif

  return t; 
}
