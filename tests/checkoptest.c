#include <glib-object.h>
#include "gegl-check-op.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 2 
#define IMAGE_OP_HEIGHT 2 

static void
test_check_op_g_object_new(Test *test)
{
  {
    GeglCheckOp * check_op = g_object_new (GEGL_TYPE_CHECK_OP, NULL);  

    ct_test(test, check_op != NULL);
    ct_test(test, GEGL_IS_CHECK_OP(check_op));
    ct_test(test, g_type_parent(GEGL_TYPE_CHECK_OP) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglCheckOp", g_type_name(GEGL_TYPE_CHECK_OP)));

    g_object_unref(check_op);
  }
}

static void
test_check_op_pixel_rgb_float(Test *test)
{
  {
    gboolean success;
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", .1, .2, .3, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    NULL);
    g_object_unref(color);

    gegl_op_apply(constant); 

    /* This is what testutils_check_op_pixel_rgb_float does */
    {
      GeglColor *test_color = g_object_new(GEGL_TYPE_COLOR, 
                                           "rgb-float", .1, .2, .3, 
                                           NULL);

      GeglOp * check_op = g_object_new(GEGL_TYPE_CHECK_OP, 
                                    "color", test_color, 
                                    "x", 0, "y", 0,
                                    "image-op", constant,
                                    NULL);
      g_object_unref(test_color);

      gegl_op_apply(check_op); 

      success = gegl_check_op_get_success(GEGL_CHECK_OP(check_op)); 

      g_object_unref(check_op);
    }

    ct_test(test, success);  

    g_object_unref(constant);
  }
}

static void
test_check_op_rgb_uint8(Test *test)
{
  {
    gboolean success;
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", 1/255.0, 2/255.0, 3/255.0, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    "image-data-type", "rgb-uint8", 
                                    NULL);
    g_object_unref(color);

    gegl_op_apply(constant); 

    /* This is what testutils_check_op_rgb_uint8 does */
    {
      GeglColor *test_color = g_object_new(GEGL_TYPE_COLOR, 
                                           "rgb-float", 1/255.0, 2/255.0, 3/255.0, 
                                           NULL);
      GeglOp * check_op = g_object_new(GEGL_TYPE_CHECK_OP, 
                                    "color", test_color, 
                                    "x", 0, "y", 0,
                                    "image-op", constant,
                                    NULL);
      g_object_unref(test_color);
      gegl_op_apply(check_op); 

      success = gegl_check_op_get_success(GEGL_CHECK_OP(check_op)); 

      g_object_unref(check_op);
    }

    ct_test(test, success);  

    g_object_unref(constant);
  }
}

static void
test_check_op_testutils_pixel_rgb_float(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", .1, .2, .3, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(constant), .1, .2, .3));  

    g_object_unref(constant);
  }
}

static void
test_check_op_testutils_rgb_uint8(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", 1/255.0, 2/255.0, 3/255.0, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    "image-data-type", "rgb-uint8",
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE_OP(constant), 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
test_check_op_testutils_pixel_rgb_float_xy(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", .1, .2, .3, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_pixel_rgb_float_xy(GEGL_IMAGE_OP(constant), 1, 1, .1, .2, .3));  

    g_object_unref(constant);
  }
}

static void
test_check_op_testutils_rgb_uint8_xy(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", 1/255.0, 2/255.0, 3/255.0, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    "image-data-type", "rgb-uint8",
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_rgb_uint8_xy(GEGL_IMAGE_OP(constant), 1, 1, 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
test_check_op_testutils_color_models_match_failure(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", 1/255.0, 2/255.0, 3/255.0, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    "image-data-type", "rgb-uint8",
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    /* returns false since check_op fails */
    ct_test(test, !testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(constant), .1, .2, .3));  

    g_object_unref(constant);
  }

  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR, 
                                    "rgb-float", .1, .2, .3, 
                                    NULL);
    GeglOp *constant = g_object_new(GEGL_TYPE_FILL, 
                                    "fill-color", color, 
                                    "image-data-type", "rgb-float",
                                    NULL);
    g_object_unref(color);
    gegl_op_apply(constant); 

    /* returns false since check_op fails */
    ct_test(test, !testutils_check_rgb_uint8(GEGL_IMAGE_OP(constant), 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
check_op_test_setup(Test *test)
{
}

static void
check_op_test_teardown(Test *test)
{
}

Test *
create_check_op_test()
{
  Test* t = ct_create("GeglCheckOpTest");

  g_assert(ct_addSetUp(t, check_op_test_setup));
  g_assert(ct_addTearDown(t, check_op_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_check_op_g_object_new));
  g_assert(ct_addTestFun(t, test_check_op_pixel_rgb_float));
  g_assert(ct_addTestFun(t, test_check_op_rgb_uint8));
  g_assert(ct_addTestFun(t, test_check_op_testutils_pixel_rgb_float));
  g_assert(ct_addTestFun(t, test_check_op_testutils_rgb_uint8));
  g_assert(ct_addTestFun(t, test_check_op_testutils_pixel_rgb_float_xy));
  g_assert(ct_addTestFun(t, test_check_op_testutils_rgb_uint8_xy));
  /*
  g_assert(ct_addTestFun(t, test_check_op_testutils_color_models_match_failure));
  */
#endif

  return t; 
}
