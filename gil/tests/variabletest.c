#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_variable_g_object_new(Test *test)
{
  {
    GilVariable * variable = g_object_new (GIL_TYPE_VARIABLE, "variable", "aVariable", NULL);

    ct_test(test, variable != NULL);
    ct_test(test, GIL_IS_VARIABLE(variable));
    ct_test(test, g_type_parent(GIL_TYPE_VARIABLE) == GIL_TYPE_EXPRESSION);
    ct_test(test, !strcmp("GilVariable", g_type_name(GIL_TYPE_VARIABLE)));

    g_object_unref(variable);
  }
}

static void
test_variable_g_object_get(Test *test)
{
  {
    GilVariable * variable = g_object_new (GIL_TYPE_VARIABLE, "variable", "bVariable", NULL);
    const gchar* name = gil_variable_get_name(variable);
    ct_test(test, 0 == strcmp(name, "bVariable"));
    g_object_unref(variable);
  }
}


static void
test_variable_g_object_set(Test *test)
{
  {
  }
}

static void
variable_setup(Test *test)
{
}

static void
variable_teardown(Test *test)
{
}

Test *
create_variable_test()
{
  Test *t = ct_create("GilVariableTest");

  g_assert(ct_addSetUp(t, variable_setup));
  g_assert(ct_addTearDown(t, variable_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_variable_g_object_new));
  g_assert(ct_addTestFun(t, test_variable_g_object_get));
  g_assert(ct_addTestFun(t, test_variable_g_object_set));
#endif

  return t;
}
