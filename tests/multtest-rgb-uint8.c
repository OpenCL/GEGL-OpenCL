#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define MULT0 1.2 
#define MULT1 1.3 
#define MULT2 1.4 
#define MULT3 1.5 

#define R0  80
#define G0 160
#define B0 240

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
                                    "source", source,
                                    "mult0", MULT0, 
                                    "mult1", MULT1, 
                                    "mult2", MULT2, 
                                    NULL);  

    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(mult)));
    ct_test(test, source == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(mult), 0));

    g_object_unref(mult);
  }

  {
    gfloat mult0, mult1, mult2;
    GeglMult * mult = g_object_new (GEGL_TYPE_MULT, 
                                    "source", source,
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
    guint8 r, g, b;
    GeglOp *mult = g_object_new(GEGL_TYPE_MULT,
                                "source", source,
                                "mult0", MULT0, 
                                "mult1", MULT1, 
                                "mult2", MULT2,
                                NULL);

    gegl_op_apply(mult); 

    r = CLAMP((gint)(R0 * MULT0 + .5), 0, 255); 
    g = CLAMP((gint)(G0 * MULT1 + .5), 0, 255); 
    b = CLAMP((gint)(B0 * MULT2 + .5), 0, 255); 

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE(mult), r, g, b)); 

    g_object_unref(mult);
  }

  {
    guint8 r, g, b;
    GeglOp *mult1 = g_object_new(GEGL_TYPE_MULT,
                                 "source", source,
                                 "mult0", MULT0, 
                                 "mult1", MULT1, 
                                 "mult2", MULT2,
                                 NULL);

    GeglOp *mult2 = g_object_new(GEGL_TYPE_MULT,
                                 "source", mult1,
                                 "mult0", MULT0, 
                                 "mult1", MULT1, 
                                 "mult2", MULT2,
                                 NULL);

    gegl_op_apply(mult2); 

    r = CLAMP((gint)(R0 * MULT0 + .5), 0, 255); 
    g = CLAMP((gint)(G0 * MULT1 + .5), 0, 255); 
    b = CLAMP((gint)(B0 * MULT2 + .5), 0, 255); 

    r = CLAMP((gint)(r * MULT0 + .5), 0, 255); 
    g = CLAMP((gint)(g * MULT1 + .5), 0, 255); 
    b = CLAMP((gint)(b * MULT2 + .5), 0, 255); 

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE(mult2), r, g, b)); 

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
                        "pixel-rgb-uint8", R0, G0, B0, 
                        NULL); 
}

static void
mult_test_teardown(Test *test)
{
  g_object_unref(source);
}

Test *
create_mult_test_rgb_uint8()
{
  Test* t = ct_create("GeglMultTestRgbUint8");

  g_assert(ct_addSetUp(t, mult_test_setup));
  g_assert(ct_addTearDown(t, mult_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_mult_g_object_new));
  g_assert(ct_addTestFun(t, test_mult_g_object_properties));
  g_assert(ct_addTestFun(t, test_mult_apply));
#endif

  return t; 
}
