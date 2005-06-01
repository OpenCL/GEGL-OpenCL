#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

static void
test_value_set(Test *test)
{
  GValue *value =  g_new0(GValue, 1);
  g_value_init(value, G_TYPE_INT);
  g_value_set_int(value, 233);

  ct_test(test, 233 == g_value_get_int(value));

  g_value_unset(value);
  g_free(value);
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
test_float_array_value_set(Test *test)
{
  gfloat array[] = {1.0,2.0,3.0};
  const gfloat *channels = NULL;
  gint length;

  GValue *value =  g_new0(GValue, 1);
  g_value_init(value, GEGL_TYPE_FLOAT_ARRAY);

  g_value_set_float_array(value, 3, &array[0]);

  channels = g_value_get_float_array(value, &length);

  ct_test(test, GEGL_FLOAT_EQUAL(1.0, channels[0]));
  ct_test(test, GEGL_FLOAT_EQUAL(2.0, channels[1]));
  ct_test(test, GEGL_FLOAT_EQUAL(3.0, channels[2]));

  g_value_unset(value);
  g_free(value);
}

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

#if 1
  g_assert(ct_addTestFun(t, test_value_set));
  g_assert(ct_addTestFun(t, test_value_compatible));
  g_assert(ct_addTestFun(t, test_float_array_value_set));
#endif

  return t;
}
