#include "gegl.h"
#include "gegl-mock-point-op.h"
#include "gegl-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_WIDTH 10 
#define IMAGE_HEIGHT 5 

#define R0 .1 
#define G0 .2
#define B0 .3 

#define R1 .4 
#define G1 .5
#define B1 .6  

static void
test_point_op_g_object_new(Test *test)
{
  {
    GeglPointOp * point_op = g_object_new (GEGL_TYPE_MOCK_POINT_OP, NULL);  

    ct_test(test, point_op != NULL);
    ct_test(test, GEGL_IS_POINT_OP(point_op));
    ct_test(test, g_type_parent(GEGL_TYPE_POINT_OP) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglPointOp", g_type_name(GEGL_TYPE_POINT_OP)));

    ct_test(test, GEGL_IS_MOCK_POINT_OP(point_op));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_POINT_OP) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglMockPointOp", g_type_name(GEGL_TYPE_MOCK_POINT_OP)));

    g_object_unref(point_op);
  }
}

static void
test_point_op_apply(Test *test)
{
  GeglRect roi = {0,0,10,10};

  GeglOp *source0 = g_object_new(GEGL_TYPE_COLOR, 
                                 "width", IMAGE_WIDTH, 
                                 "height", IMAGE_HEIGHT, 
                                 "pixel-rgb-float", R0, G0, B0, 
                                 NULL); 

  GeglOp *source1 = g_object_new(GEGL_TYPE_COLOR, 
                                 "width", IMAGE_WIDTH, 
                                 "height", IMAGE_HEIGHT, 
                                 "pixel-rgb-float", R1, G1, B1, 
                                 NULL); 

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_POINT_OP, 
                              "source0", source0,
                              "source1", source1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(source0);
  g_object_unref(source1);
}

static void
point_op_test_setup(Test *test)
{
}

static void
point_op_test_teardown(Test *test)
{
}

Test *
create_point_op_test()
{
  Test* t = ct_create("GeglPointOpTest");

  g_assert(ct_addSetUp(t, point_op_test_setup));
  g_assert(ct_addTearDown(t, point_op_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_point_op_g_object_new));
  g_assert(ct_addTestFun(t, test_point_op_apply));
#endif

  return t; 
}
