#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 2
#define IMAGE_OP_HEIGHT 2

static void
test_fill_g_object_new(Test *test)
{
  {
    GeglFill * fill = g_object_new (GEGL_TYPE_FILL, NULL);

    ct_test(test, fill != NULL);
    ct_test(test, GEGL_IS_FILL(fill));
    ct_test(test, g_type_parent(GEGL_TYPE_FILL) == GEGL_TYPE_NO_INPUT);
    ct_test(test, !strcmp("GeglFill", g_type_name(GEGL_TYPE_FILL)));

    g_object_unref(fill);
  }
}

static void
test_fill_g_object_properties(Test *test)
{
  {
    gint width;
    gint height;

    GeglColor *color = g_object_new(GEGL_TYPE_COLOR,
                                    "rgb-float", .1, .2, .3,
                                    NULL);
    GeglFill *op = g_object_new(GEGL_TYPE_FILL,
                                "width", IMAGE_OP_WIDTH,
                                "height", IMAGE_OP_HEIGHT,
                                "fill-color", color,
                                NULL);

    g_object_unref(color);

    g_object_get(op, "width", &width, NULL);
    g_object_get(op, "height", &height, NULL);

    ct_test(test, IMAGE_OP_WIDTH == width);
    ct_test(test, IMAGE_OP_HEIGHT == height);

    g_object_unref(op);
  }
}

static void
test_fill_apply(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR,
                                    "rgb-float", .1, .2, .3,
                                    NULL);
    GeglOp *op = g_object_new(GEGL_TYPE_FILL,
                              "fill-color", color,
                              NULL);
    g_object_unref(color);

    gegl_op_apply(op);

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(op), .1, .2, .3));

    g_object_unref(op);
  }
}

static void
test_fill_apply_roi(Test *test)
{
  {
    GeglRect roi = {1,1,IMAGE_OP_WIDTH/2,IMAGE_OP_HEIGHT/2};
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR,
                                    "rgb-float", .1, .2, .3,
                                    NULL);
    GeglOp *op = g_object_new(GEGL_TYPE_FILL,
                              "fill-color", color,
                              NULL);
    g_object_unref(color);

    gegl_op_apply_roi(op, &roi);

    ct_test(test, testutils_check_pixel_rgb_float_xy(GEGL_IMAGE_OP(op), 1, 1, .1, .2, .3));

    g_object_unref(op);
  }
}

static void
test_fill_apply_width_height(Test *test)
{
  {
    GeglColor *color = g_object_new(GEGL_TYPE_COLOR,
                                    "rgb-float", .1, .2, .3,
                                    NULL);
    GeglOp *op = g_object_new(GEGL_TYPE_FILL,
                              "width", IMAGE_OP_WIDTH,
                              "height", IMAGE_OP_HEIGHT,
                              "fill-color", color,
                              NULL);
    g_object_unref(color);

    gegl_op_apply(op);

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(op), .1, .2, .3));

    g_object_unref(op);
  }
}

static void
fill_test_setup(Test *test)
{
}

static void
fill_test_teardown(Test *test)
{
}

Test *
create_fill_test_float()
{
  Test* t = ct_create("GeglFillTestFloat");

  g_assert(ct_addSetUp(t, fill_test_setup));
  g_assert(ct_addTearDown(t, fill_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_fill_g_object_new));
  g_assert(ct_addTestFun(t, test_fill_g_object_properties));
  g_assert(ct_addTestFun(t, test_fill_apply));
  g_assert(ct_addTestFun(t, test_fill_apply_roi));
  g_assert(ct_addTestFun(t, test_fill_apply_width_height));
#endif

  return t;
}
