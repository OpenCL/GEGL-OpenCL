#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_stat_op_impl_g_object_new(Test *test)
{
  {
    GeglStatOpImpl * stat_op_impl = g_object_new (GEGL_TYPE_PRINT_IMPL, NULL);  

    ct_test(test, stat_op_impl != NULL);
    ct_test(test, GEGL_IS_STAT_OP_IMPL(stat_op_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_STAT_OP_IMPL) == GEGL_TYPE_OP_IMPL);
    ct_test(test, !strcmp("GeglStatOpImpl", g_type_name(GEGL_TYPE_STAT_OP_IMPL)));

    g_object_unref(stat_op_impl);
  }
}

static void
stat_op_impl_test_setup(Test *test)
{
}

static void
stat_op_impl_test_teardown(Test *test)
{
}

Test *
create_stat_op_impl_test()
{
  Test* t = ct_create("GeglStatOpImplTest");

  g_assert(ct_addSetUp(t, stat_op_impl_test_setup));
  g_assert(ct_addTearDown(t, stat_op_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_stat_op_impl_g_object_new));

  return t; 
}
