#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-op.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglColorModel * color_model;
static GeglTile *tile0, *tile1;

#define AREA0_X 1 
#define AREA0_Y 1 
#define AREA0_WIDTH 2 
#define AREA0_HEIGHT 2 
static GeglRect area0 = {AREA0_X, AREA0_Y, AREA0_WIDTH, AREA0_HEIGHT};

#define AREA1_X 2 
#define AREA1_Y 2 
#define AREA1_WIDTH 2 
#define AREA1_HEIGHT 2 
static GeglRect area1 = {AREA1_X, AREA1_Y, AREA1_WIDTH, AREA1_HEIGHT};

static GList *values;

static void
test_op_g_object_new(Test *test)
{
  {
    GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, NULL);  

    ct_test(test, op != NULL);

    ct_test(test, GEGL_IS_OP(op));
    ct_test(test, g_type_parent(GEGL_TYPE_OP) == GEGL_TYPE_NODE);
    ct_test(test, !strcmp("GeglOp", g_type_name(GEGL_TYPE_OP)));

    ct_test(test, GEGL_IS_MOCK_OP(op));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_OP) == GEGL_TYPE_OP);
    ct_test(test, !strcmp("GeglMockOp", g_type_name(GEGL_TYPE_MOCK_OP)));

    g_object_unref(op);
  }
}

static void
test_op_compute_have_rect(Test *test)
{
  GValue *output_value;
  GeglRect result_rect;
  GeglRect need_rect = {0,0,2,2};
  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              NULL);  

  /* Set the initial need rect in the output value */
  output_value = gegl_op_get_output_value(op);
  g_value_set_image_data_rect(output_value, &need_rect);

  /* Bounding box of inputs have rects, intersected with need rect */
  gegl_op_compute_have_rect(op, values);

  output_value = gegl_op_get_output_value(op);
  g_value_get_image_data_rect(output_value, &result_rect);

  /*                    
    result_rect = bbox(have_rect0, have_rect1)  intersect  need_rect 
                = bbox([1,3]x[1,3],[2,4]x[2,4]) intersect [0,2]x[0,2] 
                = [1,2]x[1,2]
     or
    result_rect = {1,1,1,1}           

  */

  ct_test(test, 1 == result_rect.x);
  ct_test(test, 1 == result_rect.y);
  ct_test(test, 1 == result_rect.w);
  ct_test(test, 1 == result_rect.h);

  g_object_unref(op);
}


static void
test_op_compute_need_rect(Test *test)
{
  GValue *output_value;
  GValue *input_value;
  GeglRect need_rect;
  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              NULL);  

  output_value = gegl_op_get_output_value(op);
  g_value_set_image_data(output_value, NULL, &area0); 

  input_value = (GValue*)g_list_nth_data(values,0);

  /*                    
    (input)need_rect = (output) need_rect  
                     = AREA0 
  */
  gegl_op_compute_need_rect(op, input_value, 0);

  g_value_get_image_data_rect(input_value, &need_rect);

  /* The bounding box */ 
  ct_test(test, AREA0_X == need_rect.x);
  ct_test(test, AREA0_Y == need_rect.y);
  ct_test(test, AREA0_WIDTH == need_rect.w);
  ct_test(test, AREA0_HEIGHT == need_rect.h);

  g_object_unref(op);
}

static void
test_op_apply(Test *test)
{
  GeglRect roi = {0,0,10,10};
  GeglOp * input0 = g_object_new (GEGL_TYPE_MOCK_OP, 
                                  "num_inputs", 0,
                                  "num_outputs", 1,
                                  NULL);  

  GeglOp * input1 = g_object_new (GEGL_TYPE_MOCK_OP, 
                                  "num_inputs", 0,
                                  "num_outputs", 1,
                                  NULL);  

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, 
                              "num_inputs", 2,
                              "num_outputs", 1,
                              "input0", input0,
                              "input1", input1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(input0);
  g_object_unref(input1);
}


static void
test_op_setup(Test *test)
{
  GValue * value;
  color_model = gegl_color_model_instance("RgbFloat");
  values = NULL;

  tile0 = g_object_new (GEGL_TYPE_TILE, 
                       "area", &area0, 
                       "colormodel", color_model,
                        NULL);  

  value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_IMAGE_DATA);
  g_value_set_image_data(value, tile0, &area0); 

  values = g_list_append(values, value);

  tile1 = g_object_new (GEGL_TYPE_TILE, 
                       "area", &area1, 
                       "colormodel", color_model,
                        NULL);  

  value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_IMAGE_DATA);
  g_value_set_image_data(value, tile1, &area1); 

  values = g_list_append(values, value);

  ct_test(test, 2 == g_list_length(values));
}

static void
test_op_teardown(Test *test)
{
  GValue * value;
  g_object_unref(color_model);
  g_object_unref(tile0);
  g_object_unref(tile1);

  value = g_list_nth_data(values, 0);
  g_value_unset(value);
  g_free(value);

  value = g_list_nth_data(values, 1);
  g_value_unset(value);
  g_free(value);

  g_list_free(values);
}

Test *
create_op_test()
{
  Test* t = ct_create("GeglOpTest");

  g_assert(ct_addSetUp(t, test_op_setup));
  g_assert(ct_addTearDown(t, test_op_teardown));
  g_assert(ct_addTestFun(t, test_op_g_object_new));
  g_assert(ct_addTestFun(t, test_op_compute_have_rect));
  g_assert(ct_addTestFun(t, test_op_compute_need_rect));
  g_assert(ct_addTestFun(t, test_op_apply));
                                     
  return t; 
}
