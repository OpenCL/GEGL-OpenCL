#include "gegl.h"
#include "gegl-mock-point-op.h"
#include "gegl-mock-node.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

GeglColorModel * color_model;
#define SAMPLED_IMAGE_WIDTH 10 
#define SAMPLED_IMAGE_HEIGHT 5 

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

  GeglSampledImage * image0 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                            "width", SAMPLED_IMAGE_WIDTH, 
                                            "height", SAMPLED_IMAGE_HEIGHT, 
                                            "colormodel", color_model,
                                             NULL);  

  GeglSampledImage * image1 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                            "width", SAMPLED_IMAGE_WIDTH, 
                                            "height", SAMPLED_IMAGE_HEIGHT, 
                                            "colormodel", color_model,
                                             NULL);  

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_POINT_OP, 
                              "source0", image0,
                              "source1", image1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(image0);
  g_object_unref(image1);
}

static void
point_op_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
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
  g_assert(ct_addTestFun(t, test_point_op_g_object_new));
  g_assert(ct_addTestFun(t, test_point_op_apply));

  return t; 
}
