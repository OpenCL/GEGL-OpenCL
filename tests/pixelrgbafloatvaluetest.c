#include <glib-object.h>
#include "gegl-mock-properties-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

static void
test_pixel_rgba_float_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  gfloat r,g,b,a;

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_set_pixel_rgba_float(value, .1, .2, .3, .4);

  g_value_get_pixel_rgba_float(value, &r, &g, &b, &a);
  ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, b));
  ct_test(test, GEGL_FLOAT_EQUAL(.4, a));

  ct_test(test, g_type_is_a(GEGL_TYPE_PIXEL_RGBA_FLOAT, GEGL_TYPE_PIXEL));
  ct_test(test, !g_type_is_a(GEGL_TYPE_PIXEL, GEGL_TYPE_PIXEL_RGBA_FLOAT));

  g_value_unset(value);
  g_free(value);
}

static void
test_pixel_rgba_float_value_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 
  gfloat r,g,b,a;

  g_value_init(dest_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_init(src_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);

  g_value_set_pixel_rgba_float(src_value, .1, .2, .3, .4);

  g_value_copy(src_value, dest_value);

  g_value_get_pixel_rgba_float(dest_value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(.1,r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2,g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3,b));
  ct_test(test, GEGL_FLOAT_EQUAL(.4,a));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

static void
test_pixel_rgba_float_value_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  gfloat r,g,b,a;

  g_value_init(src_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);

  g_value_set_pixel_rgba_float(src_value, .1, .2, .3, .4);
  g_value_set_pixel_rgba_float(dest_value, .4, .5, .6, .7);

  /* These value types are compatible ... since both float */
  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and transform just copies compatibles so dest_value becomes float .1,.2,.3 */
  ct_test(test, g_value_transform(src_value, dest_value));

  g_value_get_pixel_rgba_float(dest_value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(.1,r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2,g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3,b));
  ct_test(test, GEGL_FLOAT_EQUAL(.4,a));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_pixel_rgba_float_value_transform_rgb_float_rgba_float(Test *test)
{
  /* transform rgb to rgba */
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  gfloat r, g, b, a;

  g_value_init(src_value, GEGL_TYPE_PIXEL_RGB_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);

  /* These value types are not compatible ... */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* but they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  g_value_set_pixel_rgb_float(src_value, .5, .5, .5);

  g_value_transform(src_value, dest_value);

  g_value_get_pixel_rgba_float(dest_value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(.5, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, b));
  ct_test(test, GEGL_FLOAT_EQUAL(1.0, a));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_pixel_rgba_float_value_transform_rgba_float_rgb_float(Test *test)
{
  /* transform rgb to rgba */
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  gfloat r, g, b;

  g_value_init(src_value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_PIXEL_RGB_FLOAT);

  /* These value types are not compatible ... */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* but they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  g_value_set_pixel_rgba_float(src_value, .5, .5, .5, .5);

  g_value_transform(src_value, dest_value);

  g_value_get_pixel_rgb_float(dest_value, &r, &g, &b);

  ct_test(test, GEGL_FLOAT_EQUAL(.5, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, b));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_pixel_rgba_float_param_value_validate_false(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_pixel_rgba_float("blah", 
                                                        "Blah",
                                                        "This is rgba float data",
                                                         0.0, 1.0,
                                                         0.0, 1.0,
                                                         0.0, 1.0,
                                                         0.0, 1.0,
                                                         .1, .2, .3, .4, 
                                                         G_PARAM_READWRITE);
  gfloat r, g, b, a;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_set_pixel_rgba_float(value, .4, .5, .6, .7);

  ct_test(test, GEGL_TYPE_PIXEL_RGBA_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, !g_param_value_validate(pspec, value));

  g_value_get_pixel_rgba_float(value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(.4, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.6, b));
  ct_test(test, GEGL_FLOAT_EQUAL(.7, a));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_pixel_rgba_float_param_value_validate_true(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_pixel_rgba_float("blah", 
                                                 "Blah",
                                                 "This is rgba float data",
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  .1, .2, .3, .4, 
                                                  G_PARAM_READWRITE);
  gfloat r, g, b, a;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_set_pixel_rgba_float(value, 1.1, -.2, .3, 1.2);

  ct_test(test, GEGL_TYPE_PIXEL_RGBA_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, g_param_value_validate(pspec, value));

  g_value_get_pixel_rgba_float(value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(1.0, r));
  ct_test(test, GEGL_FLOAT_EQUAL(0.0, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, b));
  ct_test(test, GEGL_FLOAT_EQUAL(1.0, a));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_pixel_rgba_float_param_value_set_default(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_pixel_rgba_float("blah", 
                                                 "Blah",
                                                 "This is rgba float data",
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  0.0, 1.0,
                                                  .1, .2, .3, .4, 
                                                  G_PARAM_READWRITE);
  gfloat r, g, b, a;

  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_param_value_set_default(pspec, value);
  g_value_get_pixel_rgba_float(value, &r, &g, &b, &a);

  ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, b));
  ct_test(test, GEGL_FLOAT_EQUAL(.4, a));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_pixel_rgba_float_pixel_get_color_model(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GeglColorModel *color_model;

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  color_model = g_value_pixel_get_color_model(value);

  ct_test(test, color_model == gegl_color_model_instance("rgba-float"));
  g_value_unset(value);
  g_free(value);
}

static void
test_pixel_rgba_float_pixel_get_data(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  gfloat *data;

  g_value_init(value, GEGL_TYPE_PIXEL_RGBA_FLOAT);
  g_value_set_pixel_rgba_float(value, .1, .2, .3, .4);

  data = (gfloat*)g_value_pixel_get_data(value);

  ct_test(test, GEGL_FLOAT_EQUAL(.1, data[0]));
  ct_test(test, GEGL_FLOAT_EQUAL(.2, data[1]));
  ct_test(test, GEGL_FLOAT_EQUAL(.3, data[2]));
  ct_test(test, GEGL_FLOAT_EQUAL(.4, data[3]));

  g_value_unset(value);
  g_free(value);
}

static void
test_pixel_rgba_float_pixel_get_pixel_value_info(Test *test)
{
  PixelValueInfo *pixel_value_info = g_type_get_qdata(GEGL_TYPE_PIXEL_RGBA_FLOAT,
                                                      g_quark_from_string("pixel_value_info")); 

  ct_test(test, 0 == strcmp("GeglChannelFloat", pixel_value_info->channel_type_name));
  ct_test(test, 0 == strcmp("rgb", pixel_value_info->color_space_name));
  ct_test(test, TRUE == pixel_value_info->has_alpha);
}

static void
pixel_rgba_float_value_test_setup(Test *test)
{
}

static void
pixel_rgba_float_value_test_teardown(Test *test)
{
}

Test *
create_pixel_rgba_float_value_test()
{
  Test* t = ct_create("GeglPixelRgbaFloatValueTest");

  g_assert(ct_addSetUp(t, pixel_rgba_float_value_test_setup));
  g_assert(ct_addTearDown(t, pixel_rgba_float_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_value_set));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_value_copy));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_value_compatible));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_value_transform_rgb_float_rgba_float));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_value_transform_rgba_float_rgb_float));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_param_value_validate_false));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_param_value_validate_true));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_param_value_set_default));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_pixel_get_color_model));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_pixel_get_data));
  g_assert(ct_addTestFun(t, test_pixel_rgba_float_pixel_get_pixel_value_info));
#endif

  return t; 
}
