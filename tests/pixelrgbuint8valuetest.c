#include <glib-object.h>
#include "gegl-mock-properties-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

static void
test_pixel_rgb_uint8_value_set(Test *test)
{
  /* uint8 */
    GValue *value =  g_new0(GValue, 1); 
    guint8 r,g,b;

    g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
    g_value_set_pixel_rgb_uint8(value, 1, 2, 3);

    g_value_get_pixel_rgb_uint8(value, &r, &g, &b);
    ct_test(test, 1 == r);
    ct_test(test, 2 == g);
    ct_test(test, 3 == b);

    ct_test(test, g_type_is_a(GEGL_TYPE_PIXEL_RGB_UINT8, GEGL_TYPE_PIXEL));
    ct_test(test, !g_type_is_a(GEGL_TYPE_PIXEL, GEGL_TYPE_PIXEL_RGB_UINT8));

    g_value_unset(value);
    g_free(value);
}

static void
test_pixel_rgb_uint8_value_copy(Test *test)
{
  /* uint8 */
    GValue * src_value =  g_new0(GValue, 1); 
    GValue * dest_value =  g_new0(GValue, 1); 
    guint8 r,g,b;

    g_value_init(dest_value, GEGL_TYPE_PIXEL_RGB_UINT8);
    g_value_init(src_value, GEGL_TYPE_PIXEL_RGB_UINT8);

    g_value_set_pixel_rgb_uint8(src_value, 1, 2, 3);

    g_value_copy(src_value, dest_value);

    g_value_get_pixel_rgb_uint8(dest_value, &r, &g, &b);

    ct_test(test, 1 == r);
    ct_test(test, 2 == g);
    ct_test(test, 3 == b);

    g_value_unset(dest_value);
    g_value_unset(src_value);

    g_free(dest_value);
    g_free(src_value);

}

static void
test_pixel_rgb_uint8_value_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);
  guint8 r,g,b;

  g_value_init(src_value, GEGL_TYPE_PIXEL_RGB_UINT8);
  g_value_init(dest_value, GEGL_TYPE_PIXEL_RGB_UINT8);

  g_value_set_pixel_rgb_uint8(src_value, 1, 2, 3);
  g_value_set_pixel_rgb_uint8(dest_value, 4, 5, 6);

  /* These value types are compatible ... since both uint8 */
  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and transform just copies compatibles so dest_value becomes uint8 100. */
  ct_test(test, g_value_transform(src_value, dest_value));

  g_value_get_pixel_rgb_uint8(dest_value, &r, &g, &b);

  ct_test(test, 1 == r);
  ct_test(test, 2 == g);
  ct_test(test, 3 == b);

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_pixel_rgb_uint8_value_not_compatible(Test *test)
{

  /* transform float to uint8 */
    GValue * src_value = g_new0(GValue, 1);
    GValue * dest_value = g_new0(GValue, 1);
    guint8 r, g, b;

    g_value_init(src_value, GEGL_TYPE_PIXEL_RGB_FLOAT);
    g_value_init(dest_value, GEGL_TYPE_PIXEL_RGB_UINT8);

    /* These value types are not compatible ... */
    ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

    /* but they are transformable... */
    ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

    g_value_set_pixel_rgb_float(src_value, .5, .5, .5);

    g_value_transform(src_value, dest_value);

    g_value_get_pixel_rgb_uint8(dest_value, &r, &g, &b);

    ct_test(test, 128 == r);
    ct_test(test, 128 == g);
    ct_test(test, 128 == b);

    g_value_unset(src_value);
    g_value_unset(dest_value);

    g_free(src_value);
    g_free(dest_value);
}

static void
test_pixel_rgb_uint8_param_value_validate_false(Test *test)
{
    GValue *value =  g_new0(GValue, 1); 
    GParamSpec *pspec =  gegl_param_spec_pixel_rgb_uint8("blah", 
                                                         "Blah",
                                                         "This is rgb float data",
                                                          10, 245,
                                                          10, 245,
                                                          128, 128,
                                                          128, 128, 128, 
                                                          G_PARAM_READWRITE);
    guint8 r, g, b;

    g_param_spec_ref(pspec);
    g_param_spec_sink(pspec);

    g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
    g_value_set_pixel_rgb_uint8(value, 50, 60, 128);

    ct_test(test, GEGL_TYPE_PIXEL_RGB_UINT8 == G_PARAM_SPEC_VALUE_TYPE(pspec));
    ct_test(test, !g_param_value_validate(pspec, value));

    g_value_get_pixel_rgb_uint8(value, &r, &g, &b);

    ct_test(test, 50 == r);
    ct_test(test, 60 == g);
    ct_test(test, 128 == b);

    g_value_unset(value);
    g_free(value);
    g_param_spec_unref(pspec);
}

static void
test_pixel_rgb_uint8_param_value_validate_true(Test *test)
{
    GValue *value =  g_new0(GValue, 1); 
    GParamSpec *pspec =  gegl_param_spec_pixel_rgb_uint8("blah", 
                                                   "Blah",
                                                   "This is rgb float data",
                                                    10, 245,
                                                    10, 245,
                                                    128, 128,
                                                    128, 128, 128, 
                                                    G_PARAM_READWRITE);
    guint8 r, g, b;

    g_param_spec_ref(pspec);
    g_param_spec_sink(pspec);

    g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
    g_value_set_pixel_rgb_uint8(value, 0, 60, 50);

    ct_test(test, GEGL_TYPE_PIXEL_RGB_UINT8 == G_PARAM_SPEC_VALUE_TYPE(pspec));
    ct_test(test, g_param_value_validate(pspec, value));

    g_value_get_pixel_rgb_uint8(value, &r, &g, &b);

    ct_test(test, 10 == r);
    ct_test(test, 60 == g);
    ct_test(test, 128 == b);

    g_value_unset(value);
    g_free(value);
    g_param_spec_unref(pspec);
}

static void
test_pixel_rgb_uint8_param_value_set_default(Test *test)
{
    GValue *value =  g_new0(GValue, 1); 
    GParamSpec *pspec =  gegl_param_spec_pixel_rgb_uint8("blah", 
                                                   "Blah",
                                                   "This is rgb float data",
                                                    10, 245,
                                                    10, 245,
                                                    128, 128,
                                                    128, 128, 128, 
                                                    G_PARAM_READWRITE);
    guint8 r, g, b;

    g_param_spec_ref(pspec);
    g_param_spec_sink(pspec);

    g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
    g_param_value_set_default(pspec, value);

    g_value_get_pixel_rgb_uint8(value, &r, &g, &b);

    ct_test(test, 128 == r);
    ct_test(test, 128 == g);
    ct_test(test, 128 == b);

    g_value_unset(value);
    g_free(value);
    g_param_spec_unref(pspec);
}

static void
test_pixel_rgb_uint8_pixel_get_color_model(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GeglColorModel *color_model;

  g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
  color_model = g_value_pixel_get_color_model(value);

  ct_test(test, color_model == gegl_color_model_instance("rgb-uint8"));

  g_value_unset(value);
  g_free(value);
}

static void
test_pixel_rgb_uint8_pixel_get_data(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  guint8 *data;

  g_value_init(value, GEGL_TYPE_PIXEL_RGB_UINT8);
  g_value_set_pixel_rgb_uint8(value, 0, 60, 50);

  data = (guint8*)g_value_pixel_get_data(value);

  ct_test(test, 0 == data[0]);
  ct_test(test, 60 == data[1]);
  ct_test(test, 50 == data[2]);

  g_value_unset(value);
  g_free(value);
}

static void
test_pixel_rgb_uint8_pixel_get_pixel_value_info(Test *test)
{
  PixelValueInfo *pixel_value_info = g_type_get_qdata(GEGL_TYPE_PIXEL_RGB_UINT8,
                                                      g_quark_from_string("pixel_value_info")); 

  ct_test(test, 0 == strcmp("GeglChannelUInt8", pixel_value_info->channel_type_name));
  ct_test(test, 0 == strcmp("rgb", pixel_value_info->color_space_name));
  ct_test(test, FALSE == pixel_value_info->has_alpha);
}

static void
pixel_rgb_uint8_value_test_setup(Test *test)
{
}

static void
pixel_rgb_uint8_value_test_teardown(Test *test)
{
}

Test *
create_pixel_rgb_uint8_value_test()
{
  Test* t = ct_create("GeglPixelRgbUint8ValueTest");

  g_assert(ct_addSetUp(t, pixel_rgb_uint8_value_test_setup));
  g_assert(ct_addTearDown(t, pixel_rgb_uint8_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_value_set));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_value_copy));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_value_compatible));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_value_not_compatible));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_param_value_validate_false));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_param_value_validate_true));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_param_value_set_default));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_pixel_get_color_model));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_pixel_get_data));
  g_assert(ct_addTestFun(t, test_pixel_rgb_uint8_pixel_get_pixel_value_info));
#endif

  return t; 
}
