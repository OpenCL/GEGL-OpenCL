#include <glib-object.h>
#include "gegl-mock-properties-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

static void
test_channel_uint8_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_CHANNEL_UINT8);
  g_value_set_channel_uint8(value, 233);

  ct_test(test, 233 == g_value_get_channel_uint8(value));

  ct_test(test, g_type_is_a(GEGL_TYPE_CHANNEL_UINT8, GEGL_TYPE_CHANNEL));
  ct_test(test, !g_type_is_a(GEGL_TYPE_CHANNEL, GEGL_TYPE_CHANNEL_UINT8));

  g_value_unset(value);
  g_free(value);
}

static void
test_channel_uint8_value_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 

  g_value_init(dest_value, GEGL_TYPE_CHANNEL_UINT8);
  g_value_init(src_value, GEGL_TYPE_CHANNEL_UINT8);

  g_value_set_channel_uint8(src_value, 128);

  g_value_copy(src_value, dest_value);

  ct_test(test, 128 == g_value_get_channel_uint8(dest_value));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

static void
test_channel_uint8_value_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);

  g_value_init(src_value, GEGL_TYPE_CHANNEL_UINT8);
  g_value_init(dest_value, GEGL_TYPE_CHANNEL_UINT8);

  g_value_set_channel_uint8(src_value, 100);
  g_value_set_channel_uint8(dest_value, 200);

  /* These value types are compatible ... since both uint8 */
  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and transform just copies compatibles so dest_value becomes uint8 100. */
  ct_test(test, g_value_transform(src_value, dest_value));

  ct_test(test, 100 == g_value_get_channel_uint8(dest_value));
  ct_test(test, 100 == g_value_get_channel_uint8(src_value));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_channel_uint8_value_not_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);

  g_value_init(src_value, GEGL_TYPE_CHANNEL_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_CHANNEL_UINT8);

  /* These value types are not compatible ... */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* but they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  g_value_set_channel_float(src_value, .5);

  g_value_transform(src_value, dest_value);

  ct_test(test, 128 == g_value_get_channel_uint8(dest_value));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_channel_uint8_param_value_validate_false(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_channel_uint8("blah", 
                                             "Blah",
                                             "This is uint8 data",
                                              10,
                                              250,
                                              0,
                                              G_PARAM_READWRITE);

  g_value_init(value, GEGL_TYPE_CHANNEL_UINT8);
  g_value_set_channel_uint8(value, 128);

  ct_test(test, GEGL_TYPE_CHANNEL_UINT8 == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, !g_param_value_validate(pspec, value));
  ct_test(test, 128 == g_value_get_channel_uint8(value));

  g_value_unset(value);
  g_free(value);
}

static void
test_channel_uint8_param_value_validate_true(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_channel_uint8("blah", 
                                             "Blah",
                                             "This is uint8 data",
                                             128,
                                             255,
                                             0,
                                             G_PARAM_READWRITE);

  g_value_init(value, GEGL_TYPE_CHANNEL_UINT8);
  g_value_set_channel_uint8(value, 50);

  ct_test(test, GEGL_TYPE_CHANNEL_UINT8 == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, g_param_value_validate(pspec, value));
  ct_test(test, 128 == g_value_get_channel_uint8(value));

  g_value_unset(value);
  g_free(value);
}

static void
test_channel_uint8_param_value_set_default(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_channel_uint8("blah", 
                                                     "Blah",
                                                     "This is uint8 data",
                                                     10, 240,
                                                     128,
                                                     G_PARAM_READWRITE);
  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_CHANNEL_UINT8);
  g_param_value_set_default(pspec, value);

  ct_test(test, 128 == g_value_get_channel_uint8(value));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_channel_uint8_collect_values(Test *test)
{
  guint8 channel_uint8;
  GeglOp * op = g_object_new(GEGL_TYPE_MOCK_PROPERTIES_FILTER,
                             "channel-uint8", 233,
                             NULL);

  g_object_get(op, "channel-uint8", &channel_uint8, NULL);

  ct_test(test, 233 == channel_uint8);
  g_object_unref(op);
}


static void
channel_uint8_value_test_setup(Test *test)
{
}

static void
channel_uint8_value_test_teardown(Test *test)
{
}

Test *
create_channel_uint8_value_test()
{
  Test* t = ct_create("GeglChannelUint8ValueTest");

  g_assert(ct_addSetUp(t, channel_uint8_value_test_setup));
  g_assert(ct_addTearDown(t, channel_uint8_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_channel_uint8_value_set));
  g_assert(ct_addTestFun(t, test_channel_uint8_value_copy));
  g_assert(ct_addTestFun(t, test_channel_uint8_value_compatible));
  g_assert(ct_addTestFun(t, test_channel_uint8_value_not_compatible));
  g_assert(ct_addTestFun(t, test_channel_uint8_param_value_validate_false));
  g_assert(ct_addTestFun(t, test_channel_uint8_param_value_validate_true));
  g_assert(ct_addTestFun(t, test_channel_uint8_param_value_set_default));
  g_assert(ct_addTestFun(t, test_channel_uint8_collect_values));
#endif

  return t; 
}
