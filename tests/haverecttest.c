#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define IMAGE_OP_WIDTH 5 
#define IMAGE_OP_HEIGHT 5 

#define R0 .1 
#define G0 .2
#define B0 .3 

#define R1 .4 
#define G1 .5
#define B1 .6  

static void
test_haverect_color_default_size(Test *t)
{
  GeglRect default_rect = {0,0,GEGL_DEFAULT_WIDTH, GEGL_DEFAULT_HEIGHT};

  GeglRect have_rect;
  GeglData *output_data;

  GeglOp * op = g_object_new(GEGL_TYPE_COLOR, 
                             "pixel-rgb-float", R0, G0, B0, 
                             NULL); 

  gegl_image_op_compute_have_rect(GEGL_IMAGE_OP(op)); 

  output_data = gegl_op_get_nth_output_data(op, 0);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(output_data), &have_rect);

  ct_test(t, gegl_rect_equal(&have_rect, &default_rect));  

  g_object_unref(op);
}

static void
test_haverect_color(Test *t)
{
  GeglRect have_rect;
  GeglData *output_data;

  GeglOp * op = g_object_new(GEGL_TYPE_COLOR, 
                             "width", IMAGE_OP_WIDTH, 
                             "height", IMAGE_OP_HEIGHT, 
                             "pixel-rgb-float", R0, G0, B0, 
                             NULL); 

  gegl_image_op_compute_have_rect(GEGL_IMAGE_OP(op)); 

  output_data = gegl_op_get_nth_output_data(op, 0);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(output_data), &have_rect);

  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0, IMAGE_OP_WIDTH, IMAGE_OP_HEIGHT));  

  g_object_unref(op);
}

static void
test_haverect_op(Test *t)
{
  GeglRect have_rect;
  GeglRect have_rect0 = {0,0,2,2};
  GeglRect have_rect1 = {1,1,2,2};
  GeglOp * op = g_object_new (GEGL_TYPE_I_ADD, NULL);  

  GeglData *input0_data = gegl_op_get_nth_input_data(op, 0);
  GeglData *input1_data = gegl_op_get_nth_input_data(op, 1);
  GeglData *output_data;

  gegl_image_data_set_rect(GEGL_IMAGE_DATA(input0_data), &have_rect0);
  gegl_image_data_set_rect(GEGL_IMAGE_DATA(input1_data), &have_rect1);

  gegl_image_op_compute_have_rect(GEGL_IMAGE_OP(op)); 

  output_data = gegl_op_get_nth_output_data(op, 0);
  gegl_image_data_get_rect(GEGL_IMAGE_DATA(output_data), &have_rect);

  ct_test(t, gegl_rect_equal_coords(&have_rect, 0,0,3,3));  

  g_object_unref(op);
}

static void
haverect_test_setup(Test *test)
{
}

static void
haverect_test_teardown(Test *test)
{
}

Test *
create_haverect_test()
{
  Test* t = ct_create("GeglHaveRectTest");

  g_assert(ct_addSetUp(t, haverect_test_setup));
  g_assert(ct_addTearDown(t, haverect_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_haverect_color_default_size));
  g_assert(ct_addTestFun(t, test_haverect_color));
  g_assert(ct_addTestFun(t, test_haverect_op));
#endif

  return t; 
}
