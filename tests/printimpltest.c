#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_print_impl_g_object_new(Test *test)
{
  {
    GeglPrintImpl * print_impl = g_object_new (GEGL_TYPE_PRINT_IMPL, NULL);  

    ct_test(test, print_impl != NULL);
    ct_test(test, GEGL_IS_PRINT_IMPL(print_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_PRINT_IMPL) == GEGL_TYPE_STAT_OP_IMPL);
    ct_test(test, !strcmp("GeglPrintImpl", g_type_name(GEGL_TYPE_PRINT_IMPL)));

    g_object_unref(print_impl);
  }
}

static void
print_impl_test_setup(Test *test)
{
}

static void
print_impl_test_teardown(Test *test)
{
}

Test *
create_print_impl_test()
{
  Test* t = ct_create("GeglPrintImplTest");

  g_assert(ct_addSetUp(t, print_impl_test_setup));
  g_assert(ct_addTearDown(t, print_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_print_impl_g_object_new));

  return t; 
}
