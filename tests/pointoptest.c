#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static void
test_point_op_g_object_new(Test *test)
{
  {
    GeglPointOp * point_op = g_object_new (GEGL_TYPE_CONST_MULT, NULL);  

    ct_test(test, point_op != NULL);
    ct_test(test, GEGL_IS_POINT_OP(point_op));
    ct_test(test, g_type_parent(GEGL_TYPE_POINT_OP) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglPointOp", g_type_name(GEGL_TYPE_POINT_OP)));

    g_object_unref(point_op);
  }
}

static void
test_point_op_get_nth_input_offset(Test *test)
{
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

    GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                                 "source0", source0,
                                 "source1", source1,
                                 NULL);  

    ct_test(test, source0 == gegl_op_get_source0(add));
    ct_test(test, source1 == gegl_op_get_source1(add));

    {
      GeglPoint in_point = {1,2}; 
      GeglPoint out_point; 

      gegl_point_op_set_nth_input_offset(GEGL_POINT_OP(add), 1, &in_point); 
      gegl_point_op_get_nth_input_offset(GEGL_POINT_OP(add), 1, &out_point); 

      ct_test(test, out_point.x == in_point.x); 
      ct_test(test, out_point.y == in_point.y); 
      ct_test(test, 1 == out_point.x); 
      ct_test(test, 2 == out_point.y); 
    }

    g_object_unref(add);
    g_object_unref(rgb_float);
    g_object_unref(source0);
    g_object_unref(source1);
  }
}

static void
test_point_op_compute_have_rect(Test *test)
{
  GeglOp * op = g_object_new (GEGL_TYPE_ADD, NULL);  

  GList * inputs_have_rects = NULL; 
  GeglRect have_rect0 = {0,0,30,30};
  GeglRect have_rect1 = {10,10,30,30};
  GeglRect have_rect;

  inputs_have_rects = g_list_append(inputs_have_rects, &have_rect0); 
  inputs_have_rects = g_list_append(inputs_have_rects, &have_rect1); 

  gegl_image_compute_have_rect(GEGL_IMAGE(op), &have_rect, inputs_have_rects);


  ct_test(test, have_rect.x == 10 && 
                have_rect.y == 10 && 
                have_rect.w == 20 && 
                have_rect.h == 20);

  g_object_unref(op);

  g_list_free(inputs_have_rects);
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
  g_assert(ct_addTestFun(t, test_point_op_g_object_new));
  g_assert(ct_addTestFun(t, test_point_op_get_nth_input_offset));
  g_assert(ct_addTestFun(t, test_point_op_compute_have_rect));

  return t; 
}
