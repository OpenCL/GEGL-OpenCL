#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

#define AREA_WIDTH 10 
#define AREA_HEIGHT 10 
static GeglImage * rgb_image;  

static void
test_value_image_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_IMAGE);
  g_value_set_object(value, rgb_image);

  ct_test(test, rgb_image == g_value_get_object(value));
  ct_test(test, g_value_type_compatible(GEGL_TYPE_IMAGE, G_TYPE_OBJECT));
  ct_test(test, !g_value_type_compatible(G_TYPE_OBJECT, GEGL_TYPE_IMAGE));

  g_value_unset(value);

  g_free(value);
}

static void
test_value_image_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 

  g_value_init(dest_value, GEGL_TYPE_IMAGE);
  g_value_init(src_value, GEGL_TYPE_IMAGE);

  g_value_set_object(src_value, rgb_image);

  g_value_copy(src_value, dest_value);

  ct_test(test, rgb_image == g_value_get_object(dest_value));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

#if 0
static void
test_value_image_param_spec_validate(Test *test)
{
  GeglImage *converted_image;
  gfloat converted_val;
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglRect smaller_area = {0,0,AREA_WIDTH-1,AREA_HEIGHT};
  GeglRect bigger_area = {0,0,AREA_WIDTH+1,AREA_HEIGHT};

  GeglColorModel *pixel_rgb_float = gegl_color_model_instance("rgb-float");
  GeglColorModel *gray_float = gegl_color_model_instance("gray-float");

  GeglOp *filled = testutils_pixel_rgb_float_sampled_image_op(AREA_WIDTH, 
                                                     AREA_HEIGHT, 
                                                     .1, .2, .3);
  GeglImage *image = gegl_image_op_get_image(GEGL_IMAGE_OP(filled));

  GParamSpec *pspec0 = gegl_param_spec_image("data0",
                                            "Data0",
                                            "data0",
                                            &smaller_area,
                                            pixel_rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec1 = gegl_param_spec_image("data1",
                                            "Data1",
                                            "data1",
                                            &bigger_area,
                                            pixel_rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec2 = gegl_param_spec_image("data2",
                                            "Data2",
                                            "data2",
                                            &area,
                                            gray_float,
                                            G_PARAM_READWRITE);

  GValue * value =  g_new0(GValue, 1); 
  g_value_init(value, GEGL_TYPE_IMAGE);

  /* image rect contains spec rect */
  g_value_set_image(value, image);
  ct_test(test, !g_param_value_validate(pspec0, value)); /*returns true if modified*/

  /* image rect does not contain spec rect, cant validate */
  g_value_set_image(value, image);
  ct_test(test, g_param_value_validate(pspec1, value));
  ct_test(test, NULL == g_value_get_image(value));

  /* color models are different, can validate after converting */
  g_value_set_image(value, image);
  ct_test(test, g_param_value_validate(pspec2, value));

  /* This was an gray to rgb conversion */
  converted_val = .3*.1 + .59*.2 + .11*.3;

  converted_image = g_value_get_image(value);
  ct_test(test, NULL != converted_image);

  ct_test(test, testutils_check_pixel_image(converted_image,  "grayfloat", 
                                           converted_val, 0, 0, 0));

  g_value_unset(value);
  g_free(value);

  g_object_unref(filled);

  g_param_spec_unref(pspec0);
  g_param_spec_unref(pspec1);
  g_param_spec_unref(pspec2);
}
#endif

static void
value_image_test_setup(Test *test)
{
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
  rgb_image = g_object_new (GEGL_TYPE_IMAGE, NULL);  
  gegl_image_create_tile(rgb_image, pixel_rgb_float, &area);
}

static void
value_image_test_teardown(Test *test)
{
  g_object_unref(rgb_image);
}

Test *
create_value_image_test()
{
  Test* t = ct_create("GeglValueImageTest");

  g_assert(ct_addSetUp(t, value_image_test_setup));
  g_assert(ct_addTearDown(t, value_image_test_teardown));
  g_assert(ct_addTestFun(t, test_value_image_set));
  g_assert(ct_addTestFun(t, test_value_image_copy));
#if 0
  g_assert(ct_addTestFun(t, test_value_image_compatible));
  g_assert(ct_addTestFun(t, test_value_image_param_spec_validate));
#endif

  return t; 
}
