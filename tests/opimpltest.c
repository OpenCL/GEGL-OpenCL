#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_op_impl_g_object_new(Test *test)
{
  {
    GeglOpImpl * op_impl = g_object_new (GEGL_TYPE_OP_IMPL, NULL);  

    ct_test(test, op_impl != NULL);
    ct_test(test, GEGL_IS_OP_IMPL(op_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_OP_IMPL) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglOpImpl", g_type_name(GEGL_TYPE_OP_IMPL)));

    g_object_unref(op_impl);
  }
}

static void
op_impl_test_setup(Test *test)
{
}

static void
op_impl_test_teardown(Test *test)
{
}

Test *
create_op_impl_test()
{
  Test* t = ct_create("GeglOpImplTest");

  g_assert(ct_addSetUp(t, op_impl_test_setup));
  g_assert(ct_addTearDown(t, op_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_op_impl_g_object_new));

  return t; 
}
