#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define MULTIPLIER .5

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglOp * source;
static GeglOp * dest;

static void
test_const_mult_g_object_new(Test *test)
{
  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, NULL);  

    ct_test(test, const_mult != NULL);
    ct_test(test, GEGL_IS_CONST_MULT(const_mult));
    ct_test(test, g_type_parent(GEGL_TYPE_CONST_MULT) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglConstMult", g_type_name(GEGL_TYPE_CONST_MULT)));

    g_object_unref(const_mult);
  }

  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, 
                                               "multiplier", MULTIPLIER, 
                                               NULL);  

    ct_test(test, const_mult != NULL);
    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(const_mult)));
    ct_test(test, MULTIPLIER == gegl_const_mult_get_multiplier(const_mult));

    g_object_unref(const_mult);
  }

  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, 
                                               "multiplier", MULTIPLIER, 
                                               "input", source,
                                               NULL);  

    ct_test(test, const_mult != NULL);
    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(const_mult)));
    ct_test(test, MULTIPLIER == gegl_const_mult_get_multiplier(const_mult));
    ct_test(test, source == (GeglOp*)gegl_node_get_nth_input(GEGL_NODE(const_mult), 0));

    g_object_unref(const_mult);
  }
}

static void
test_const_mult_g_object_set(Test *test)
{
  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, NULL);  

    ct_test(test, const_mult != NULL);

    g_object_set(const_mult, "multiplier", MULTIPLIER, NULL);

    ct_test(test, MULTIPLIER == gegl_const_mult_get_multiplier(const_mult));

    g_object_unref(const_mult);
  }
}

static void
test_const_mult_g_object_get(Test *test)
{
  {
    gfloat multiplier;
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, 
                                               "multiplier", MULTIPLIER, 
                                               NULL);  

    ct_test(test, const_mult != NULL);

    g_object_get(const_mult, "multiplier", &multiplier, NULL);

    ct_test(test, MULTIPLIER == multiplier);

    g_object_unref(const_mult);
  }
}

static void
test_const_mult_apply(Test *test)
{
  {
    GeglOp *const_mult = g_object_new(GEGL_TYPE_CONST_MULT,
                                      "input", source,
                                      "multiplier", MULTIPLIER,
                                      NULL);

    gegl_op_apply_image(const_mult, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .1 * MULTIPLIER, 
                                                             .2 * MULTIPLIER, 
                                                             .3 * MULTIPLIER));  
    g_object_unref(const_mult);
  }

  {
    GeglOp *const_mult1 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "multiplier", MULTIPLIER,
                                       "input", source,
                                       NULL);

    GeglOp *const_mult2 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "multiplier", MULTIPLIER,
                                       "input", const_mult1,
                                       NULL);

    gegl_op_apply_image(const_mult2, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), .1 * MULTIPLIER * MULTIPLIER, 
                                                             .2 * MULTIPLIER * MULTIPLIER, 
                                                             .3 * MULTIPLIER * MULTIPLIER));  

    g_object_unref(const_mult1);
    g_object_unref(const_mult2);
  }

  {
    GeglOp *const_mult1 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "multiplier", MULTIPLIER,
                                       "input", source,
                                       NULL);

    gegl_op_apply_image(const_mult1, NULL, NULL); 
    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(const_mult1), .1 * MULTIPLIER, 
                                                                    .2 * MULTIPLIER, 
                                                                    .3 * MULTIPLIER));  

    g_object_unref(const_mult1);
  }
}

static void
const_mult_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
  source = testutils_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                        SAMPLED_IMAGE_HEIGHT, 
                                       .1, .2, .3);

  ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(source), .1, .2, .3));  
  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

  g_object_unref(rgb_float);
}

static void
const_mult_test_teardown(Test *test)
{
  g_object_unref(source);
  g_object_unref(dest);
}

Test *
create_const_mult_test()
{
  Test* t = ct_create("GeglConstMultTest");

  g_assert(ct_addSetUp(t, const_mult_test_setup));
  g_assert(ct_addTearDown(t, const_mult_test_teardown));
  g_assert(ct_addTestFun(t, test_const_mult_g_object_new));
  g_assert(ct_addTestFun(t, test_const_mult_g_object_set));
  g_assert(ct_addTestFun(t, test_const_mult_g_object_get));
  g_assert(ct_addTestFun(t, test_const_mult_apply));

  return t; 
}
