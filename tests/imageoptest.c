#include "gegl.h"
#include "gegl-mock-image-op.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_image_op_g_object_new(Test *test)
{
  GeglImageOp * image_op = g_object_new (GEGL_TYPE_MOCK_IMAGE_OP, NULL);  

  ct_test(test, image_op != NULL);
  ct_test(test, GEGL_IS_IMAGE_OP(image_op));
  ct_test(test, g_type_parent(GEGL_TYPE_IMAGE_OP) == GEGL_TYPE_FILTER);
  ct_test(test, !strcmp("GeglImageOp", g_type_name(GEGL_TYPE_IMAGE_OP)));

  ct_test(test, GEGL_IS_MOCK_IMAGE_OP(image_op));
  ct_test(test, g_type_parent(GEGL_TYPE_MOCK_IMAGE_OP) == GEGL_TYPE_IMAGE_OP);
  ct_test(test, !strcmp("GeglMockImageOp", g_type_name(GEGL_TYPE_MOCK_IMAGE_OP)));

  g_object_unref(image_op);
}

static void
test_image_op_sources_outputs(Test *test)
{
  GeglImageOp * image_op = g_object_new (GEGL_TYPE_MOCK_IMAGE_OP, NULL);  

  ct_test(test, 0 == gegl_node_get_num_inputs(GEGL_NODE(image_op))); 
  ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(image_op))); 

  g_object_unref(image_op);
}

static void
image_op_test_setup(Test *test)
{
}

static void
image_op_test_teardown(Test *test)
{
}

Test *
create_image_op_test()
{
  Test* t = ct_create("GeglImageOpTest");

  g_assert(ct_addSetUp(t, image_op_test_setup));
  g_assert(ct_addTearDown(t, image_op_test_teardown));
  g_assert(ct_addTestFun(t, test_image_op_g_object_new));
  g_assert(ct_addTestFun(t, test_image_op_sources_outputs));

  return t; 
}
