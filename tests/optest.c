#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static void
test_op_g_object_new(Test *test)
{
  {
    GeglOp * op = g_object_new (GEGL_TYPE_OP, NULL);  

    ct_test(test, op != NULL);
    ct_test(test, GEGL_IS_OP(op));
    ct_test(test, g_type_parent(GEGL_TYPE_OP) == GEGL_TYPE_NODE);
    ct_test(test, !strcmp("GeglOp", g_type_name(GEGL_TYPE_OP)));

    g_object_unref(op);
  }
}

static void
test_op_get_source(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  GeglOp * source0 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                  "colormodel", rgb_float,
                                  "width", SAMPLED_IMAGE_WIDTH, 
                                  "height", SAMPLED_IMAGE_HEIGHT,
                                  NULL);  

  GeglOp * source1 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                  "colormodel", rgb_float,
                                  "width", SAMPLED_IMAGE_WIDTH, 
                                  "height", SAMPLED_IMAGE_HEIGHT,
                                  NULL);  

  {
    GeglOp * op = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                "colormodel", rgb_float,
                                "width", SAMPLED_IMAGE_WIDTH, 
                                "height", SAMPLED_IMAGE_HEIGHT,
                                NULL);  

    ct_test(test, 0 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, NULL == gegl_op_get_source0(op));
    ct_test(test, NULL == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_CONST_MULT,
                                NULL);  

    ct_test(test, 1 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, NULL == gegl_op_get_source0(op));
    ct_test(test, NULL == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_CONST_MULT,
                                "source0", source0,
                                NULL);  

    ct_test(test, 1 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, source0 == gegl_op_get_source0(op));
    ct_test(test, NULL == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_ADD,
                                NULL);  

    ct_test(test, 2 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, NULL == gegl_op_get_source0(op));
    ct_test(test, NULL == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_ADD,
                                "source0", source0,
                                NULL);  

    ct_test(test, 2 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, source0 == gegl_op_get_source0(op));
    ct_test(test, NULL == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_ADD,
                                "source1", source1,
                                NULL);  

    ct_test(test, 2 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, NULL == gegl_op_get_source0(op));
    ct_test(test, source1 == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  {
    GeglOp * op = g_object_new (GEGL_TYPE_ADD,
                                "source0", source0,
                                "source1", source1,
                                NULL);  

    ct_test(test, 2 == gegl_node_num_inputs(GEGL_NODE(op)));

    ct_test(test, source0 == gegl_op_get_source0(op));
    ct_test(test, source1 == gegl_op_get_source1(op));

    g_object_unref(op);
  }

  g_object_unref(rgb_float);
  g_object_unref(source0);
  g_object_unref(source1);
}

static void
test_op_g_object_get_set(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  GeglOp * source1 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                  "colormodel", rgb_float,
                                  "width", SAMPLED_IMAGE_WIDTH, 
                                  "height", SAMPLED_IMAGE_HEIGHT,
                                  NULL);  

  GeglOp * op = g_object_new (GEGL_TYPE_ADD, 
                              "source1", source1, 
                              NULL);  

  ct_test(test, 2 == gegl_node_num_inputs(GEGL_NODE(op)));

  {
    GeglOp *src1 = NULL;

    g_object_get (op, "source1", &src1, NULL);  

    ct_test(test, src1 == source1);

    g_object_unref(src1);
  }

  {
    GeglOp * src1 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                  "colormodel", rgb_float,
                                  "width", SAMPLED_IMAGE_WIDTH, 
                                  "height", SAMPLED_IMAGE_HEIGHT,
                                  NULL);  

    ct_test(test, op != NULL);

    g_object_set (op, "source1", src1, NULL);  

    ct_test(test, src1 == gegl_op_get_source1(op));

    g_object_unref(src1);
  }

  {
    ct_test(test, op != NULL);

    g_object_set (op, "source1", NULL, NULL);  

    ct_test(test, NULL == gegl_op_get_source1(op));
  }

  g_object_unref(op);
  g_object_unref(rgb_float);
  g_object_unref(source1);
}


static void
op_test_setup(Test *test)
{
}

static void
op_test_teardown(Test *test)
{
}

Test *
create_op_test()
{
  Test* t = ct_create("GeglOpTest");

  g_assert(ct_addSetUp(t, op_test_setup));
  g_assert(ct_addTearDown(t, op_test_teardown));
  g_assert(ct_addTestFun(t, test_op_g_object_new));
  g_assert(ct_addTestFun(t, test_op_g_object_get_set));
  g_assert(ct_addTestFun(t, test_op_get_source));

  return t; 
}
