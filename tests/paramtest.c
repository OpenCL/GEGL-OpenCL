#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  g_value_init(value, GEGL_TYPE_UINT8);
  g_value_set_int(value, 233);

  ct_test(test, 233 == g_value_get_int(value));
  ct_test(test, g_value_type_compatible(GEGL_TYPE_UINT8, G_TYPE_INT));
  ct_test(test, !g_value_type_compatible(G_TYPE_INT, GEGL_TYPE_UINT8));

  g_value_unset(value);
  g_free(value);
}

GValue * gegl_value_uint8_new(guint8 val)
{
  GValue *value =  g_new0(GValue, 1); 
  g_value_init(value, GEGL_TYPE_UINT8);
  g_value_set_int(value, val);
  return value;
}

void 
gegl_value_uint8_delete(GValue *value)
{
  g_value_unset(value);
  g_free(value);
}

#if 0
static void
test_value_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 

  g_value_init(dest_value, GEGL_TYPE_UINT8);
  g_value_init(src_value, GEGL_TYPE_UINT8);

  g_value_set_object(src_value, rgb_image_buffer);

  g_value_copy(src_value, dest_value);

  ct_test(test, rgb_image_buffer == g_value_get_object(dest_value));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

static void
test_value_compatible(Test *test)
{
  GValue * float_value = g_new0(GValue, 1);
  GValue * int_value = g_new0(GValue, 1);

  g_value_init(float_value, G_TYPE_FLOAT);
  g_value_init(int_value, G_TYPE_INT);

  /* These value types are not compatible ... */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(int_value), G_VALUE_TYPE(float_value)));
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(float_value), G_VALUE_TYPE(int_value)));

  /* but they are transformable */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(float_value), G_VALUE_TYPE(int_value)));
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(int_value), G_VALUE_TYPE(float_value)));

  /* transform float to int, just truncates */
  g_value_set_float(float_value, 3.6);
  ct_test(test, g_value_transform(float_value, int_value));
  ct_test(test, 3 == g_value_get_int(int_value));

  /* transform int to float */
  g_value_set_int(int_value, 9);
  ct_test(test, g_value_transform(int_value, float_value));
  ct_test(test, 9 == g_value_get_float(float_value));

  g_value_unset(float_value);
  g_value_unset(int_value);

  g_free(float_value);
  g_free(int_value);
}

static void
test_value_param_spec_validate(Test *test)
{
  GeglImageBuffer *converted_image_buffer;
  gfloat converted_val;
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglRect smaller_area = {0,0,AREA_WIDTH-1,AREA_HEIGHT};
  GeglRect bigger_area = {0,0,AREA_WIDTH+1,AREA_HEIGHT};

  GeglColorModel *pixel_rgb_float = gegl_color_model_instance("rgbfloat");
  GeglColorModel *gray_float = gegl_color_model_instance("grayfloat");

  GeglOp *filled = testutils_pixel_rgb_float_sampled_image(AREA_WIDTH, 
                                                     AREA_HEIGHT, 
                                                     .1, .2, .3);
  GeglImageBuffer *image_buffer = gegl_image_get_image_buffer(GEGL_IMAGE(filled));

  GParamSpec *pspec0 = gegl_param_spec_image_buffer("data0",
                                            "Data0",
                                            "data0",
                                            &smaller_area,
                                            pixel_rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec1 = gegl_param_spec_image_buffer("data1",
                                            "Data1",
                                            "data1",
                                            &bigger_area,
                                            pixel_rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec2 = gegl_param_spec_image_buffer("data2",
                                            "Data2",
                                            "data2",
                                            &area,
                                            gray_float,
                                            G_PARAM_READWRITE);

  GValue * value =  g_new0(GValue, 1); 
  g_value_init(value, GEGL_TYPE_IMAGE_BUFFER);

  /* image_buffer rect contains spec rect */
  g_value_set_image_buffer(value, image_buffer);
  ct_test(test, !g_param_value_validate(pspec0, value)); /*returns true if modified*/

  /* image_buffer rect does not contain spec rect, cant validate */
  g_value_set_image_buffer(value, image_buffer);
  ct_test(test, g_param_value_validate(pspec1, value));
  ct_test(test, NULL == g_value_get_image_buffer(value));

  /* color models are different, can validate after converting */
  g_value_set_image_buffer(value, image_buffer);
  ct_test(test, g_param_value_validate(pspec2, value));

  /* This was an gray to rgb conversion */
  converted_val = .3*.1 + .59*.2 + .11*.3;

  converted_image_buffer = g_value_get_image_buffer(value);
  ct_test(test, NULL != converted_image_buffer);

  ct_test(test, testutils_check_pixel_image_buffer(converted_image_buffer,  "grayfloat", 
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
value_test_setup(Test *test)
{
}

static void
value_test_teardown(Test *test)
{
}

Test *
create_value_test()
{
  Test* t = ct_create("GeglValueTest");

  g_assert(ct_addSetUp(t, value_test_setup));
  g_assert(ct_addTearDown(t, value_test_teardown));

  g_assert(ct_addTestFun(t, test_value_set));

#if 0
  g_assert(ct_addTestFun(t, test_value_set));
  g_assert(ct_addTestFun(t, test_value_copy));
  g_assert(ct_addTestFun(t, test_value_compatible));
  g_assert(ct_addTestFun(t, test_value_param_spec_validate));
#endif

  return t; 
}
