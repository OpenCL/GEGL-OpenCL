#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_needrect_op(Test *t)
{
  GeglRect need_rect;
  GeglRect input0_need_rect;
  GeglRect input1_need_rect;
  GeglData *output_data;
  GeglData *input_data;

  GeglOp * op = g_object_new (GEGL_TYPE_I_ADD, NULL);  
 
  gegl_rect_set(&need_rect, 0,0,5,5);
  output_data = gegl_op_get_nth_output_data(GEGL_OP(op), 0);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(output_data), &need_rect);

  gegl_image_op_interface_compute_need_rects(GEGL_IMAGE_OP_INTERFACE(op));

  input_data = gegl_op_get_nth_input_data(GEGL_OP(op), 0);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &input0_need_rect);

  ct_test(t, gegl_rect_equal_coords(&input0_need_rect, 0,0,5,5));  

  input_data = gegl_op_get_nth_input_data(GEGL_OP(op), 1);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &input1_need_rect);

  ct_test(t, gegl_rect_equal_coords(&input1_need_rect, 0,0,5,5));  

  g_object_unref(op);
}


static void
test_needrect_op_source_needrect_set(Test *t)
{
  GeglRect need_rect;
  GeglRect input0_need_rect;
  GeglRect input1_need_rect;
  GeglData *output_data;
  GeglData *input_data;

  GeglOp * op = g_object_new (GEGL_TYPE_I_ADD, NULL);  
 
  gegl_rect_set(&need_rect, 1,1,5,5);
  output_data = gegl_op_get_nth_output_data(GEGL_OP(op), 0);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(output_data), &need_rect);

  gegl_rect_set(&input0_need_rect, 0,0,3,3);
  input_data = gegl_op_get_nth_input_data(GEGL_OP(op), 0);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(input_data), &input0_need_rect);

  gegl_image_op_interface_compute_need_rects(GEGL_IMAGE_OP_INTERFACE(op));

  gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &input0_need_rect);
  ct_test(t, gegl_rect_equal_coords(&input0_need_rect, 0,0,6,6));  

  gegl_rect_set(&input1_need_rect, 2,2,6,6);
  input_data = gegl_op_get_nth_input_data(GEGL_OP(op), 1);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(input_data), &input1_need_rect);
  gegl_image_op_interface_compute_need_rects(GEGL_IMAGE_OP_INTERFACE(op));

  gegl_image_data_get_rect(GEGL_IMAGE_DATA(input_data), &input1_need_rect);
  ct_test(t, gegl_rect_equal_coords(&input1_need_rect, 1,1,7,7));  

  g_object_unref(op);
}

static void
needrect_test_setup(Test *test)
{
}

static void
needrect_test_teardown(Test *test)
{
}

Test *
create_needrect_test()
{
  Test* t = ct_create("GeglNeedRectTest");

  g_assert(ct_addSetUp(t, needrect_test_setup));
  g_assert(ct_addTearDown(t, needrect_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_needrect_op));
  g_assert(ct_addTestFun(t, test_needrect_op_source_needrect_set));
#endif

  return t; 
}
