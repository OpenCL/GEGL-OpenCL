#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglOp *source0, *source1;
static GeglOp *dest;

static GeglOp *source0_gray, *source1_gray;
static GeglOp *dest_gray;

static GeglOp *source0_rgba, *source1_rgba;
static GeglOp *dest_rgba;

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

  {
    GeglOp * multiply =  g_object_new (GEGL_TYPE_MULTIPLY, NULL);  

    ct_test(test, NULL == gegl_node_get_source_node(GEGL_NODE(multiply), 0));
    ct_test(test, NULL == gegl_node_get_source_node(GEGL_NODE(multiply), 1));
    ct_test(test, 2 == gegl_node_get_num_inputs(GEGL_NODE(multiply)));
    ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(multiply)));

    g_object_unref(multiply);
  }

  {
    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0,
                                      "source1", source1,
                                      NULL);  

    ct_test(test, source0 == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(multiply), 0));
    ct_test(test, source1 == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(multiply), 1));

    g_object_unref(multiply);
  }
}

static void
test_multiply_apply_rgb_float(Test *test)
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

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(multiply), "RgbFloat", 
                                        .04, .10, .18, 0));
    g_object_unref(multiply);
  }
}

static void
test_multiply_apply_image_rgb_float(Test *test)
{
    /* 
       dest = source0 * source1 
       (.04,.10,.18) = (.1,.2,.3) * (.4,.5,.6)
    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0,
                                      "source1", source1,
                                      NULL);  

    gegl_op_apply_image(multiply, dest, NULL); 

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(dest), "RgbFloat", 
                                        .04, .10, .18, 0));

    g_object_unref(multiply);
}

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

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(multiply), "RgbAlphaFloat", 
                                        .26, .42, .6, .76));
    g_object_unref(multiply);
  }
}

static void
test_multiply_apply_image_rgba_float(Test *test)
{
    /* 
       source1 = (f, fa) = (.4,.5,.6,.4)
       source0 = (b, ba) = (.1,.2,.3,.6)

       d = (1-fa)*b + (1-ba)*f + f*b;
       da = fa + ba - fa*ba;

       1 - fa = .6;
       1 - ba = .4;

       (d, da) = (.6*b + .4*f + f*b, .4 + .6 - .4*.6)
               = (.26, .42, .6, .76) 
    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0_rgba,          /* Bg */
                                      "source1", source1_rgba,          /* Fg */
                                      NULL);  

    gegl_op_apply_image(multiply, dest_rgba, NULL); 

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(dest_rgba), "RgbAlphaFloat", 
                                        .26, .42, .6, .76));

    g_object_unref(multiply);
}

static void
test_multiply_apply_gray_float(Test *test)
{
  {
    /* 
       multiply = source0_gray * source1_gray 
       (.04) = (.1) * (.4)
    */

    GeglOp * multiply = g_object_new (GEGL_TYPE_MULTIPLY, 
                                      "source0", source0_gray,
                                      "source1", source1_gray,
                                      NULL);  

    gegl_op_apply(multiply); 

    ct_test(test, testutils_check_pixel(GEGL_IMAGE(multiply), "GrayFloat",
                                        .04, 0, 0, 0));
    g_object_unref(multiply);
  }
}

static void
setup_rgba_images(Test *test)
{
  GeglColorModel *rgba_float = gegl_color_model_instance("RgbAlphaFloat");


  source0_rgba = testutils_sampled_image("RgbAlphaFloat",
                                         SAMPLED_IMAGE_WIDTH, 
                                         SAMPLED_IMAGE_HEIGHT, 
                                         .1, .2, .3, .6);

  source1_rgba = testutils_sampled_image("RgbAlphaFloat",
                                         SAMPLED_IMAGE_WIDTH, 
                                         SAMPLED_IMAGE_HEIGHT, 
                                         .4, .5, .6, .4);

  dest_rgba = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                            "colormodel", rgba_float,
                            "width", SAMPLED_IMAGE_WIDTH, 
                            "height", SAMPLED_IMAGE_HEIGHT,
                            NULL);  


}

static void
teardown_rgba_images(Test * test) 
{
  g_object_unref(source0_rgba);
  g_object_unref(source1_rgba);
  g_object_unref(dest_rgba);
}

static void
setup_rgb_images(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");


  source0 = testutils_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                        SAMPLED_IMAGE_HEIGHT, 
                                        .1, .2, .3);

  source1 = testutils_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                        SAMPLED_IMAGE_HEIGHT, 
                                        .4, .5, .6);

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  


}

static void
teardown_rgb_images(Test * test) 
{
  g_object_unref(source0);
  g_object_unref(source1);
  g_object_unref(dest);
}

static void
setup_gray_images(Test *test)
{
  GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");



  source0_gray = testutils_sampled_image("GrayFloat",
                                          SAMPLED_IMAGE_WIDTH, SAMPLED_IMAGE_HEIGHT, 
                                          .1, 0, 0, 0); 

  source1_gray = testutils_sampled_image("GrayFloat",
                                          SAMPLED_IMAGE_WIDTH, SAMPLED_IMAGE_HEIGHT, 
                                          .4, 0, 0, 0); 


  dest_gray = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", gray_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  


}

static void
teardown_gray_images(Test * test) 
{
  g_object_unref(source0_gray);
  g_object_unref(source1_gray);
  g_object_unref(dest_gray);
}

static void
multiply_test_setup(Test *test)
{
  setup_rgb_images(test);
  setup_rgba_images(test);
  setup_gray_images(test);
}

static void
multiply_test_teardown(Test *test)
{
  teardown_rgb_images(test);
  teardown_rgba_images(test);
  teardown_gray_images(test);
}

Test *
create_multiply_test()
{
  Test* t = ct_create("GeglMultiplyTest");

  g_assert(ct_addSetUp(t, multiply_test_setup));
  g_assert(ct_addTearDown(t, multiply_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_multiply_g_object_new));
  g_assert(ct_addTestFun(t, test_multiply_apply_rgb_float));
  g_assert(ct_addTestFun(t, test_multiply_apply_image_rgb_float));
  g_assert(ct_addTestFun(t, test_multiply_apply_rgba_float));
  g_assert(ct_addTestFun(t, test_multiply_apply_image_rgba_float));
  g_assert(ct_addTestFun(t, test_multiply_apply_gray_float));
#endif

  return t; 
}
