#include <glib-object.h>
#include "gegl.h"
#include "gegl-check.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_WIDTH 2 
#define IMAGE_HEIGHT 2 

static void
test_color_g_object_new(Test *test)
{
  {
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, NULL);  

    ct_test(test, color != NULL);
    ct_test(test, GEGL_IS_COLOR(color));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR) == GEGL_TYPE_NO_INPUT);
    ct_test(test, !strcmp("GeglColor", g_type_name(GEGL_TYPE_COLOR)));

    g_object_unref(color);
  }
}

static void
test_color_g_object_properties(Test *test)
{
  {
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "pixel-rgb-float", .1, .2, .3, 
                                      NULL);  

    gfloat r, g, b;

    g_object_get(color, "pixel-rgb-float", &r, &g, &b, NULL);


    ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, b));

    g_object_unref(color);
  }

  {
    GeglColor *op = g_object_new(GEGL_TYPE_COLOR, 
                                 "width", IMAGE_WIDTH, 
                                 "height", IMAGE_HEIGHT, 
                                 "pixel-rgb-float",.1, .2, .3, 
                                 NULL); 

    gint width = gegl_color_get_width(op);
    gint height = gegl_color_get_height(op);

    ct_test(test, IMAGE_WIDTH == width);
    ct_test(test, IMAGE_HEIGHT == height);

    g_object_unref(op);
  }
}

static void
test_color_apply(Test *test)
{
  {
    GeglOp *op = g_object_new(GEGL_TYPE_COLOR, 
                              "pixel-rgb-float", .1, .2, .3, 
                              NULL);

    gegl_op_apply(op); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(op), .1, .2, .3));  

    g_object_unref(op);
  }
}

static void
test_color_apply_roi(Test *test)
{
  {
    GeglRect roi = {1,1,IMAGE_WIDTH/2,IMAGE_HEIGHT/2};
    GeglOp *op = g_object_new(GEGL_TYPE_COLOR, 
                              "pixel-rgb-float", .1, .2, .3,
                              NULL);

    gegl_op_apply_roi(op, &roi); 

    ct_test(test, testutils_check_pixel_rgb_float_xy(GEGL_IMAGE(op), 1, 1, .1, .2, .3));  

    g_object_unref(op);
  }
}

static void
test_color_apply_width_height(Test *test)
{
  {
    GeglOp *op = g_object_new(GEGL_TYPE_COLOR, 
                              "width", IMAGE_WIDTH, 
                              "height", IMAGE_HEIGHT, 
                              "pixel-rgb-float",.1, .2, .3, 
                              NULL); 

    gegl_op_apply(op); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(op), .1, .2, .3));  

    g_object_unref(op);
  }
}

static void
color_test_setup(Test *test)
{
}

static void
color_test_teardown(Test *test)
{
}

Test *
create_color_test_rgb_float()
{
  Test* t = ct_create("GeglColorTestRgbFloat");

  g_assert(ct_addSetUp(t, color_test_setup));
  g_assert(ct_addTearDown(t, color_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_color_g_object_new));
  g_assert(ct_addTestFun(t, test_color_g_object_properties));
  g_assert(ct_addTestFun(t, test_color_apply));
  g_assert(ct_addTestFun(t, test_color_apply_roi));
  g_assert(ct_addTestFun(t, test_color_apply_width_height));
#endif

  return t; 
}
