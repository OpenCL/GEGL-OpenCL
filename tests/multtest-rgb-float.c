#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define MULT0 .5
#define MULT1 .6
#define MULT2 .7

#define IMAGE_WIDTH 1 
#define IMAGE_HEIGHT 1 

static GeglOp * source;

static void
test_mult_g_object_new(Test *test)
{
  {
    GeglMult * mult = g_object_new (GEGL_TYPE_MULT, NULL);  

    ct_test(test, mult != NULL);
    ct_test(test, GEGL_IS_MULT(mult));
    ct_test(test, g_type_parent(GEGL_TYPE_MULT) == GEGL_TYPE_UNARY);
    ct_test(test, !strcmp("GeglMult", g_type_name(GEGL_TYPE_MULT)));

    g_object_unref(mult);
  }
}

static void
test_mult_g_object_properties(Test *test)
{
  {
    GeglMult * mult = g_object_new (GEGL_TYPE_MULT, 
                                    "input", 0, source,
                                    "mult0", MULT0, 
                                    "mult1", MULT1, 
                                    "mult2", MULT2, 
                                    NULL);  

    ct_test(test, 5 == gegl_node_get_num_inputs(GEGL_NODE(mult)));
    ct_test(test, source == (GeglOp*)gegl_node_get_source(GEGL_NODE(mult), 0));

    g_object_unref(mult);
  }

  {
    gfloat mult0, mult1, mult2;
    GeglMult * mult = g_object_new (GEGL_TYPE_MULT, 
                                    "input", 0, source,
                                    "mult0", MULT0, 
                                    "mult1", MULT1, 
                                    "mult2", MULT2, 
                                    NULL);  

    g_object_get(mult, 
                 "mult0", &mult0, 
                 "mult1", &mult1, 
                 "mult2", &mult2, 
                 NULL);

    ct_test(test, GEGL_FLOAT_EQUAL(MULT0, mult0)); 
    ct_test(test, GEGL_FLOAT_EQUAL(MULT1, mult1)); 
    ct_test(test, GEGL_FLOAT_EQUAL(MULT2, mult2)); 

    g_object_unref(mult);
  }
}

static void
test_mult_apply(Test *test)
{
  {
    GeglOp *mult = g_object_new(GEGL_TYPE_MULT,
                                "input", 0, source,
                                "mult0", MULT0, 
                                "mult1", MULT1, 
                                "mult2", MULT2,
                                NULL);

    gegl_op_apply(mult); 
    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(mult), 
                                            .1 * MULT0, 
                                            .2 * MULT1, 
                                            .3 * MULT2));  
    g_object_unref(mult);
  }

  {
    GeglOp *mult1 = g_object_new(GEGL_TYPE_MULT,
                                 "input", 0, source,
                                 "mult0", MULT0, 
                                 "mult1", MULT1, 
                                 "mult2", MULT2,
                                 NULL);

    GeglOp *mult2 = g_object_new(GEGL_TYPE_MULT,
                                 "input", 0, mult1,
                                 "mult0", MULT0, 
                                 "mult1", MULT1, 
                                 "mult2", MULT2,
                                 NULL);

    gegl_op_apply(mult2); 
    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE(mult2), 
                                            .1 * MULT0 * MULT0, 
                                            .2 * MULT1 * MULT1, 
                                            .3 * MULT2 * MULT2));  

    g_object_unref(mult1);
    g_object_unref(mult2);
  }
}

static void
mult_test_setup(Test *test)
{
  source = g_object_new(GEGL_TYPE_COLOR, 
                        "width", IMAGE_WIDTH, 
                        "height", IMAGE_HEIGHT, 
                        "pixel-rgb-float",.1, .2, .3, 
                        NULL); 
}

static void
mult_test_teardown(Test *test)
{
  g_object_unref(source);
}

Test *
create_mult_test_rgb_float()
{
  Test* t = ct_create("GeglMultTestRgbFloat");

  g_assert(ct_addSetUp(t, mult_test_setup));
  g_assert(ct_addTearDown(t, mult_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_mult_g_object_new));
  g_assert(ct_addTestFun(t, test_mult_g_object_properties));
  g_assert(ct_addTestFun(t, test_mult_apply));
#endif

  return t; 
}
