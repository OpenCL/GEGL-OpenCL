#include <glib-object.h>
#include "gegl-mock-filter.h"
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_float_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_FLOAT);
  g_value_set_gegl_float(value, .33);

  ct_test(test, GEGL_FLOAT_EQUAL(.33, g_value_get_gegl_float(value)));

  ct_test(test, g_type_is_a(GEGL_TYPE_FLOAT, GEGL_TYPE_CHANNEL));
  ct_test(test, !g_type_is_a(GEGL_TYPE_CHANNEL, GEGL_TYPE_FLOAT));

  g_value_unset(value);
  g_free(value);
}

static void
test_float_value_copy(Test *test)
{
  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 

  g_value_init(dest_value, GEGL_TYPE_FLOAT);
  g_value_init(src_value, GEGL_TYPE_FLOAT);

  g_value_set_gegl_float(src_value, .33);

  g_value_copy(src_value, dest_value);

  ct_test(test, GEGL_FLOAT_EQUAL(.33 ,g_value_get_gegl_float(dest_value)));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);
}

static void
test_float_value_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);

  g_value_init(src_value, GEGL_TYPE_FLOAT);
  g_value_init(dest_value, GEGL_TYPE_FLOAT);

  g_value_set_gegl_float(src_value, .1);
  g_value_set_gegl_float(dest_value, .2);

  /* These value types are compatible ... since both float */
  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* and transform just copies compatibles so dest_value becomes uint8 .1. */
  ct_test(test, g_value_transform(src_value, dest_value));

  ct_test(test, GEGL_FLOAT_EQUAL(.1 , g_value_get_gegl_float(dest_value)));
  ct_test(test, GEGL_FLOAT_EQUAL(.1 , g_value_get_gegl_float(src_value)));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_float_value_not_compatible(Test *test)
{
  GValue * src_value = g_new0(GValue, 1);
  GValue * dest_value = g_new0(GValue, 1);

  g_value_init(src_value, GEGL_TYPE_UINT8);
  g_value_init(dest_value, GEGL_TYPE_FLOAT);

  /* These value types are compatible ... since both uint8 */
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  /* but they are transformable... */
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));

  g_value_set_gegl_uint8(src_value, 128);

  g_value_transform(src_value, dest_value);

  ct_test(test, GEGL_FLOAT_EQUAL(.501961, g_value_get_gegl_float(dest_value)));

  g_value_unset(src_value);
  g_value_unset(dest_value);

  g_free(src_value);
  g_free(dest_value);
}

static void
test_float_param_value_validate_false(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_float("blah", 
                                             "Blah",
                                             "This is float data",
                                              0.0,
                                              1.0,
                                              .5,
                                              G_PARAM_READWRITE);

  g_value_init(value, GEGL_TYPE_FLOAT);
  g_value_set_gegl_float(value, .25);

  ct_test(test, GEGL_TYPE_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, !g_param_value_validate(pspec, value));
  ct_test(test, GEGL_FLOAT_EQUAL(.25, g_value_get_gegl_float(value)));

  g_value_unset(value);
  g_free(value);
}

static void
test_float_param_value_validate_true(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_float("blah", 
                                             "Blah",
                                             "This is float data",
                                             .5,
                                             1.0,
                                             .75,
                                             G_PARAM_READWRITE);

  g_value_init(value, GEGL_TYPE_FLOAT);
  g_value_set_gegl_float(value, .25);

  ct_test(test, GEGL_TYPE_FLOAT == G_PARAM_SPEC_VALUE_TYPE(pspec));
  ct_test(test, g_param_value_validate(pspec, value));
  ct_test(test, GEGL_FLOAT_EQUAL(.5, g_value_get_gegl_float(value)));

  g_value_unset(value);
  g_free(value);
}

static void
test_float_param_value_set_default(Test *test)
{
  GValue *value =  g_new0(GValue, 1); 
  GParamSpec *pspec =  gegl_param_spec_float("blah", 
                                             "Blah",
                                             "This is float data",
                                             0.0, 1.0,
                                             .1,
                                             G_PARAM_READWRITE);
  g_param_spec_ref(pspec);
  g_param_spec_sink(pspec);

  g_value_init(value, GEGL_TYPE_FLOAT);
  g_param_value_set_default(pspec, value);

  ct_test(test, GEGL_FLOAT_EQUAL(.1, g_value_get_gegl_float(value)));

  g_value_unset(value);
  g_free(value);
  g_param_spec_unref(pspec);
}

static void
test_float_collect_values(Test *test)
{
  GeglOp * op = g_object_new(GEGL_TYPE_MOCK_FILTER,
                             "factor-float", .5,
                             NULL);
  g_object_unref(op);
}


static void
float_value_test_setup(Test *test)
{
}

static void
float_value_test_teardown(Test *test)
{
}

Test *
create_float_value_test()
{
  Test* t = ct_create("GeglFloatValueTest");

  g_assert(ct_addSetUp(t, float_value_test_setup));
  g_assert(ct_addTearDown(t, float_value_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_float_value_set));
  g_assert(ct_addTestFun(t, test_float_value_copy));
  g_assert(ct_addTestFun(t, test_float_value_compatible));
  g_assert(ct_addTestFun(t, test_float_value_not_compatible));
  g_assert(ct_addTestFun(t, test_float_param_value_validate_false));
  g_assert(ct_addTestFun(t, test_float_param_value_validate_true));
  g_assert(ct_addTestFun(t, test_float_param_value_set_default));
  g_assert(ct_addTestFun(t, test_float_collect_values));
#endif

  return t; 
}
