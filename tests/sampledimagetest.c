#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-op.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

GeglColorModel * color_model;
#define SAMPLED_IMAGE_WIDTH 10 
#define SAMPLED_IMAGE_HEIGHT 5 

static void
test_sampled_image_g_object_new(Test *test)
{
  {
    GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                     "width", SAMPLED_IMAGE_WIDTH, 
                                                     "height", SAMPLED_IMAGE_HEIGHT, 
                                                     "colormodel", color_model,
                                                      NULL);  

    ct_test(test, sampled_image != NULL);
    ct_test(test, GEGL_IS_SAMPLED_IMAGE(sampled_image));
    ct_test(test, g_type_parent(GEGL_TYPE_SAMPLED_IMAGE) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglSampledImage", g_type_name(GEGL_TYPE_SAMPLED_IMAGE)));

    g_object_unref(sampled_image);
  }
}

static void
test_sampled_image_g_object_get(Test *test)
{
  gint width, height;
  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                   "width", SAMPLED_IMAGE_WIDTH, 
                                                   "height", SAMPLED_IMAGE_HEIGHT, 
                                                   "colormodel", color_model,
                                                    NULL);  
  g_object_get(sampled_image, 
               "width", &width, 
               "height", &height, 
               NULL);

  ct_test(test, SAMPLED_IMAGE_WIDTH == width);
  ct_test(test, SAMPLED_IMAGE_HEIGHT == height);

  g_object_unref(sampled_image);
}

static void
test_sampled_image_compute_have_rect(Test *test)
{
  GValue *output_value;
  GeglRect result_rect;
  GeglRect need_rect = {0,0,5,10};
  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                   "width", SAMPLED_IMAGE_WIDTH, 
                                                   "height", SAMPLED_IMAGE_HEIGHT, 
                                                   "colormodel", color_model,
                                                    NULL);  

  /* Set the initial need rect in the output value */
  output_value = gegl_op_get_output_value(GEGL_OP(sampled_image));
  g_value_set_image_data_rect(output_value, &need_rect);

  /* Bounding box of inputs have rects, intersected with need rect */
  gegl_op_compute_have_rect(GEGL_OP(sampled_image), NULL);

  output_value = gegl_op_get_output_value(GEGL_OP(sampled_image));
  g_value_get_image_data_rect(output_value, &result_rect);

  /*                    
    result_rect = have_rect  intersect  need_rect 
                = [0,10]x[0,5] intersect [0,5]x[0,10]
                = [0,5]x[0,5]
     or
    result_rect = {0,0,5,5}           

  */

  ct_test(test, 0 == result_rect.x);
  ct_test(test, 0 == result_rect.y);
  ct_test(test, 5 == result_rect.w);
  ct_test(test, 5 == result_rect.h);

  g_object_unref(sampled_image);
}

static void
test_sampled_image_input_apply(Test *test)
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

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, 
                              "num_inputs", 2, 
                              "input0", image0,
                              "input1", image1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(image0);
  g_object_unref(image1);
}

static void
sampled_image_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
}

static void
sampled_image_test_teardown(Test *test)
{
  g_object_unref(color_model);
}

Test *
create_sampled_image_test()
{
  Test* t = ct_create("GeglSampledImageTest");

  g_assert(ct_addSetUp(t, sampled_image_test_setup));
  g_assert(ct_addTearDown(t, sampled_image_test_teardown));
  g_assert(ct_addTestFun(t, test_sampled_image_input_apply));
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_new));
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_get));
  g_assert(ct_addTestFun(t, test_sampled_image_compute_have_rect));

  return t; 
}
