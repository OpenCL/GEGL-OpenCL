#include <glib-object.h>
#include "gegl-mock-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_rgb_float_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  gfloat r,g,b;

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  g_value_set_gegl_rgb_float(value, .1, .2, .3);

  g_value_get_gegl_rgb_float(value, &r, &g, &b);
  ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, b));

  ct_test(test, g_type_is_a(GEGL_TYPE_RGB_FLOAT, GEGL_TYPE_PIXEL));
  ct_test(test, !g_type_is_a(GEGL_TYPE_PIXEL, GEGL_TYPE_RGB_FLOAT));

  g_value_unset(value);
  g_free(value);
}

static void
test_rgb_float_value_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 
  gfloat r,g,b;

  g_value_init(dest_value, GEGL_TYPE_RGB_FLOAT);
  g_value_init(src_value, GEGL_TYPE_RGB_FLOAT);

  g_value_set_gegl_rgb_float(src_value, .1, .2, .3);

  g_value_copy(src_value, dest_value);

  g_value_get_gegl_rgb_float(dest_value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.1,r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2,g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3,b));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

static void
test_rgb_float_value_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  gfloat r,g,b;

  g_value_init(src_value, GEGL_TYPE_RGB_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_RGB_FLOAT);

  g_value_set_gegl_rgb_float(src_value, .1, .2, .3);
  g_value_set_gegl_rgb_float(dest_value, .4, .5, .6);

  /* These value types are compatible ... since both float */
  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and transform just copies compatibles so dest_value becomes float .1,.2,.3 */
  ct_test(test, g_value_transform(src_value, dest_value));

  g_value_get_gegl_rgb_float(dest_value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.1,r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2,g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3,b));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_rgb_float_value_not_compatible(Test *test)
{
  /* transform uint8 to float */
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  gfloat r, g, b;

  g_value_init(src_value, GEGL_TYPE_RGB_UINT8);
  g_value_init(dest_value, GEGL_TYPE_RGB_FLOAT);

  /* These value types are not compatible ... */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* but they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  g_value_set_gegl_rgb_uint8(src_value, 128, 128, 128);

  g_value_transform(src_value, dest_value);

  g_value_get_gegl_rgb_float(dest_value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.501961, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.501961, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.501961, b));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_rgb_float_param_value_validate_false(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_rgb_float("blah", 
                                                 "Blah",
                                                 "This is rgb float data",
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  .1, .2, .3, 
                                                  G_PARAM_READWRITE);
  gfloat r, g, b;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  g_value_set_gegl_rgb_float(value, .4, .5, .6);

  ct_test(test, GEGL_TYPE_RGB_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, !g_param_value_validate(pspec, value));

  g_value_get_gegl_rgb_float(value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.4, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.6, b));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_rgb_float_param_value_validate_true(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_rgb_float("blah", 
                                                 "Blah",
                                                 "This is rgb float data",
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  .1, .2, .3, 
                                                  G_PARAM_READWRITE);
  gfloat r, g, b;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  g_value_set_gegl_rgb_float(value, 1.1, -.1, .6);

  ct_test(test, GEGL_TYPE_RGB_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, g_param_value_validate(pspec, value));

  g_value_get_gegl_rgb_float(value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(1.0, r));
  ct_test(test, GEGL_FLOAT_EQUAL(0.0, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.6, b));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_rgb_float_param_value_set_default(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_rgb_float("blah", 
                                                 "Blah",
                                                 "This is rgb float data",
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  .1, .2, .3, 
                                                  G_PARAM_READWRITE);
  gfloat r, g, b;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  g_param_value_set_default(pspec, value);
  g_value_get_gegl_rgb_float(value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, b));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_rgb_float_collect_values(Test *test)
{
  GeglOp * op = g_object_new(GEGL_TYPE_MOCK_FILTER,
                             "pixel-rgb-float", .1, .2, .3,
                             NULL);
  g_object_unref(op);
}

static void
test_rgb_float_pixel_get_color_model(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GeglColorModel *color_model;

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  color_model = g_value_pixel_get_color_model(value);

  ct_test(test, color_model == gegl_color_model_instance("rgb-float"));
  g_value_unset(value);
  g_free(value);
}

static void
test_rgb_float_pixel_get_data(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  gfloat *data;

  g_value_init(value, GEGL_TYPE_RGB_FLOAT);
  g_value_set_gegl_rgb_float(value, .1, .2, .3);

  data = (gfloat*)g_value_pixel_get_data(value);

  ct_test(test, GEGL_FLOAT_EQUAL(.1, data[0]));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, data[1]));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, data[2]));

  g_value_unset(value);
  g_free(value);
}

static void
rgb_float_value_test_setup(Test *test)
{
}

static void
rgb_float_value_test_teardown(Test *test)
{
}

Test *
create_rgb_float_value_test()
{
  Test* t = ct_create("GeglRgbFloatValueTest");

  g_assert(ct_addSetUp(t, rgb_float_value_test_setup));
  g_assert(ct_addTearDown(t, rgb_float_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_rgb_float_value_set));
  g_assert(ct_addTestFun(t, test_rgb_float_value_copy));
  g_assert(ct_addTestFun(t, test_rgb_float_value_compatible));
  g_assert(ct_addTestFun(t, test_rgb_float_value_not_compatible));
  g_assert(ct_addTestFun(t, test_rgb_float_param_value_validate_false));
  g_assert(ct_addTestFun(t, test_rgb_float_param_value_validate_true));
  g_assert(ct_addTestFun(t, test_rgb_float_param_value_set_default));
  g_assert(ct_addTestFun(t, test_rgb_float_collect_values));
  g_assert(ct_addTestFun(t, test_rgb_float_pixel_get_color_model));
  g_assert(ct_addTestFun(t, test_rgb_float_pixel_get_data));
#endif

  return t; 
}
