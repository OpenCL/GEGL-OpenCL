#include <glib-object.h>
#include "gegl-check.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_WIDTH 2 
#define IMAGE_HEIGHT 2 

static void
test_check_g_object_new(Test *test)
{
  {
    GeglCheck * check = g_object_new (GEGL_TYPE_CHECK, NULL);  

    ct_test(test, check != NULL);
    ct_test(test, GEGL_IS_CHECK(check));
    ct_test(test, g_type_parent(GEGL_TYPE_CHECK) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglCheck", g_type_name(GEGL_TYPE_CHECK)));

    g_object_unref(check);
  }
}

static void
test_check_pixel_rgb_float(Test *test)
{
  {
    gboolean success;
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-float", .1, .2, .3, 
                                    NULL);

    gegl_op_apply(constant); 

    /* This is what testutils_check_pixel_rgb_float does */
    {
      GeglImageData * image_data = gegl_image_get_image_data(GEGL_IMAGE(constant));
      GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                    "pixel-rgb-float", .1, .2, .3, 
                                    "x", 0, "y", 0,
                                    "image_data", image_data,
                                    NULL);
      gegl_op_apply(check); 

      success = gegl_check_get_success(GEGL_CHECK(check)); 

      g_object_unref(check);
    }

    ct_test(test, success);  

    g_object_unref(constant);
  }
}

static void
test_check_rgb_uint8(Test *test)
{
  {
    gboolean success;
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-uint8", 1, 2, 3, 
                                    NULL);

    gegl_op_apply(constant); 

    /* This is what testutils_check_rgb_uint8 does */
    {
      GeglImageData * image_data = gegl_image_get_image_data(GEGL_IMAGE(constant));
      GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                    "pixel-rgb-uint8", 1, 2, 3, 
                                    "x", 0, "y", 0,
                                    "image_data", image_data,
                                    NULL);
      gegl_op_apply(check); 

      success = gegl_check_get_success(GEGL_CHECK(check)); 

      g_object_unref(check);
    }

    ct_test(test, success);  

    g_object_unref(constant);
  }
}

static void
test_check_testutils_pixel_rgb_float(Test *test)
{
  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-float", .1, .2, .3, 
                                    NULL);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(constant), .1, .2, .3));  

    g_object_unref(constant);
  }
}

static void
test_check_testutils_rgb_uint8(Test *test)
{
  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-uint8", 1, 2, 3, 
                                    NULL);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE(constant), 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
test_check_testutils_pixel_rgb_float_xy(Test *test)
{
  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-float", .1, .2, .3, 
                                    NULL);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_pixel_rgb_float_xy(GEGL_IMAGE(constant), 1, 1, .1, .2, .3));  

    g_object_unref(constant);
  }
}

static void
test_check_testutils_rgb_uint8_xy(Test *test)
{
  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-uint8", 1, 2, 3, 
                                    NULL);
    gegl_op_apply(constant); 

    ct_test(test, testutils_check_rgb_uint8_xy(GEGL_IMAGE(constant), 1, 1, 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
test_check_testutils_color_modelels_match_failure(Test *test)
{
  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-uint8", 1, 2, 3, 
                                    NULL);
    gegl_op_apply(constant); 

    /* returns false since check fails */
    ct_test(test, !testutils_check_pixel_rgb_float(GEGL_IMAGE(constant), .1, .2, .3));  

    g_object_unref(constant);
  }

  {
    GeglOp *constant = g_object_new(GEGL_TYPE_COLOR, 
                                    "pixel-rgb-float", .1, .2, .3, 
                                    NULL);
    gegl_op_apply(constant); 

    /* returns false since check fails */
    ct_test(test, !testutils_check_rgb_uint8(GEGL_IMAGE(constant), 1, 2, 3));  

    g_object_unref(constant);
  }
}

static void
check_test_setup(Test *test)
{
}

static void
check_test_teardown(Test *test)
{
}

Test *
create_check_test()
{
  Test* t = ct_create("GeglCheckTest");

  g_assert(ct_addSetUp(t, check_test_setup));
  g_assert(ct_addTearDown(t, check_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_check_g_object_new));
  g_assert(ct_addTestFun(t, test_check_pixel_rgb_float));
  g_assert(ct_addTestFun(t, test_check_rgb_uint8));
  g_assert(ct_addTestFun(t, test_check_testutils_pixel_rgb_float));
  g_assert(ct_addTestFun(t, test_check_testutils_rgb_uint8));
  g_assert(ct_addTestFun(t, test_check_testutils_pixel_rgb_float_xy));
  g_assert(ct_addTestFun(t, test_check_testutils_rgb_uint8_xy));
  g_assert(ct_addTestFun(t, test_check_testutils_color_modelels_match_failure));
#endif

  return t; 
}
