#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_float_array_set(Test *test)
{
  {
    gfloat component_values[4] = {.5,.5,.5,.5};
    const gfloat *values;
    gint length;

    GValue * value = g_new0(GValue, 1);
    g_value_init(value, GEGL_TYPE_FLOAT_ARRAY);

    g_value_set_float_array(value, 4, &component_values[0]); 

    values = g_value_get_float_array(value, &length); 


    ct_test(test, 4 == length);
    ct_test(test, GEGL_FLOAT_EQUAL(.5, values[0]));
    ct_test(test, GEGL_FLOAT_EQUAL(.5, values[1]));
    ct_test(test, GEGL_FLOAT_EQUAL(.5, values[2]));
    ct_test(test, GEGL_FLOAT_EQUAL(.5, values[3]));

    g_value_unset(value);
    g_free(value);
  }
}

static void
float_array_test_setup(Test *test)
{
}

static void
float_array_test_teardown(Test *test)
{
}

Test *
create_float_array_test()
{
  Test* t = ct_create("GeglFloatArrayTest");

  g_assert(ct_addSetUp(t, float_array_test_setup));
  g_assert(ct_addTearDown(t, float_array_test_teardown));
  g_assert(ct_addTestFun(t, test_float_array_set));

  return t; 
}
