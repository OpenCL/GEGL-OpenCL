#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define MULT0 .5
#define MULT1 .6
#define MULT2 .7
#define MULT3 .8
#define MULT4 .9

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
    ct_test(test, g_type_parent(GEGL_TYPE_CONST_MULT) == GEGL_TYPE_UNARY);
    ct_test(test, !strcmp("GeglConstMult", g_type_name(GEGL_TYPE_CONST_MULT)));

    g_object_unref(const_mult);
  }

  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, 
                                               "mult0", MULT0, 
                                               "mult1", MULT1, 
                                               "mult2", MULT2, 
                                               "mult3", MULT3, 
                                               "mult4", MULT4, 
                                               NULL);  
    ct_test(test, const_mult != NULL);
    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(const_mult)));
    g_object_unref(const_mult);
  }

  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, 
                                               "mult0", MULT0, 
                                               "mult1", MULT1, 
                                               "mult2", MULT2, 
                                               "mult3", MULT3, 
                                               "mult4", MULT4, 
                                               "source", source,
                                               NULL);  

    ct_test(test, const_mult != NULL);
    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(const_mult)));
    ct_test(test, source == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(const_mult), 0));

    g_object_unref(const_mult);
  }
}

static void
test_const_mult_g_object_set(Test *test)
{
  {
    GeglConstMult * const_mult = g_object_new (GEGL_TYPE_CONST_MULT, NULL);  
    gfloat mult0, mult1, mult2, mult3, mult4;

    ct_test(test, const_mult != NULL);

    g_object_set(const_mult, 
                 "mult0", MULT0, 
                 "mult1", MULT1, 
                 "mult2", MULT2, 
                 "mult3", MULT3, 
                 "mult4", MULT4, 
                 NULL);

    g_object_get(const_mult, 
                 "mult0", &mult0,
                 "mult1", &mult1,
                 "mult2", &mult2,
                 "mult3", &mult3,
                 "mult4", &mult4,
                 NULL);

    ct_test(test, GEGL_FLOAT_EQUAL(MULT0, mult0));
    ct_test(test, GEGL_FLOAT_EQUAL(MULT1, mult1));
    ct_test(test, GEGL_FLOAT_EQUAL(MULT2, mult2));
    ct_test(test, GEGL_FLOAT_EQUAL(MULT3, mult3));
    ct_test(test, GEGL_FLOAT_EQUAL(MULT4, mult4));

    g_object_unref(const_mult);
  }
}

static void
test_const_mult_apply(Test *test)
{
  {
    GeglOp *const_mult = g_object_new(GEGL_TYPE_CONST_MULT,
                                      "source", source,
                                      "mult0", MULT0,
                                      "mult1", MULT1,
                                      "mult2", MULT2,
                                      NULL);

    gegl_op_apply(const_mult); 
    gegl_op_apply_image(const_mult, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), 
                                                  .1 * MULT0, 
                                                  .2 * MULT1, 
                                                  .3 * MULT2));  
    g_object_unref(const_mult);
  }

  {
    GeglOp *const_mult1 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "mult0", MULT0,
                                       "mult1", MULT1,
                                       "mult2", MULT2,
                                       "source", source,
                                       NULL);

    GeglOp *const_mult2 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "mult0", MULT0,
                                       "mult1", MULT1,
                                       "mult2", MULT2,
                                       "source", const_mult1,
                                       NULL);

    gegl_op_apply_image(const_mult2, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), 
                                                  .1 * MULT0 * MULT0, 
                                                  .2 * MULT1 * MULT1, 
                                                  .3 * MULT2 * MULT2));  

    g_object_unref(const_mult1);
    g_object_unref(const_mult2);
  }

  {
    GeglOp *const_mult1 = g_object_new(GEGL_TYPE_CONST_MULT,
                                       "mult0", MULT0,
                                       "mult1", MULT1,
                                       "mult2", MULT2,
                                       "source", source,
                                       NULL);

    gegl_op_apply_image(const_mult1, NULL, NULL); 
    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(const_mult1), 
                                                  .1 * MULT0, 
                                                  .2 * MULT1, 
                                                  .3 * MULT2));  

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

#if 1 
  g_assert(ct_addTestFun(t, test_const_mult_g_object_new));
  g_assert(ct_addTestFun(t, test_const_mult_g_object_set));
  g_assert(ct_addTestFun(t, test_const_mult_apply));
#endif

  return t; 
}
