#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

#define R0 80 
#define G0 160 
#define B0 240 

#define R1 30 
#define G1 50 
#define B1 70  

static GeglOp *source0, *source1;

static void
test_i_mult_g_object_new(Test *test)
{
  {
    GeglOp * i_mult = g_object_new (GEGL_TYPE_I_MULT, NULL);  

    ct_test(test, GEGL_IS_I_MULT(i_mult));
    ct_test(test, g_type_parent(GEGL_TYPE_BINARY) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglIMult", g_type_name(GEGL_TYPE_I_MULT)));

    g_object_unref(i_mult);
  }

  {
    GeglOp * i_mult =  g_object_new (GEGL_TYPE_I_MULT, NULL);  

    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(i_mult), 0));
    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(i_mult), 1));
    ct_test(test, 3 == gegl_node_get_num_inputs(GEGL_NODE(i_mult)));
    ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(i_mult)));

    g_object_unref(i_mult);
  }
}

static void
test_i_mult_properties(Test *test)
{
  {
    GeglOp * i_mult = g_object_new (GEGL_TYPE_I_MULT, 
                                   "input", 0, source0,
                                   "input", 1, source1,
                                   NULL);  

    ct_test(test, source0 == (GeglOp*)gegl_node_get_source(GEGL_NODE(i_mult), 0));
    ct_test(test, source1 == (GeglOp*)gegl_node_get_source(GEGL_NODE(i_mult), 1));

    g_object_unref(i_mult);
  }
}

static void
test_i_mult_apply(Test *test)
{
  {
    guint r, g, b;
    guint t;
    /* 
       i_mult = source0 * source1 
       (9,31,66) =  (80,160,240) * (30,50,70)
    */

    GeglOp * i_mult = g_object_new (GEGL_TYPE_I_MULT, 
                                   "input", 0, source0,
                                   "input", 1, source1,
                                   NULL);  

    gegl_op_apply(i_mult); 

    r = INT_MULT(R0, R1, t);
    g = INT_MULT(G0, G1, t);
    b = INT_MULT(B0, B1, t);

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE_OP(i_mult), r, g, b));
    g_object_unref(i_mult);
  }
}

static void
i_mult_test_setup(Test *test)
{
  source0 = g_object_new(GEGL_TYPE_COLOR, 
                         "width", IMAGE_OP_WIDTH, 
                         "height", IMAGE_OP_HEIGHT, 
                         "pixel-rgb-uint8", R0, G0, B0, 
                         NULL); 

  source1 = g_object_new(GEGL_TYPE_COLOR, 
                         "width", IMAGE_OP_WIDTH, 
                         "height", IMAGE_OP_HEIGHT, 
                         "pixel-rgb-uint8", R1, G1, B1, 
                         NULL); 
}

static void
i_mult_test_teardown(Test *test)
{
  g_object_unref(source0);
  g_object_unref(source1);
}

Test *
create_i_mult_test_rgb_uint8()
{
  Test* t = ct_create("GeglIMultTestRgbUint8");

  g_assert(ct_addSetUp(t, i_mult_test_setup));
  g_assert(ct_addTearDown(t, i_mult_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_i_mult_g_object_new));
  g_assert(ct_addTestFun(t, test_i_mult_properties));
  g_assert(ct_addTestFun(t, test_i_mult_apply));
#endif

  return t; 
}
