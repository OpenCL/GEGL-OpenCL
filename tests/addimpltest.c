#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_add_impl_g_object_new(Test *test)
{
  {
    GeglAddImpl * add_impl = g_object_new (GEGL_TYPE_ADD_IMPL, NULL);  

    ct_test(test, add_impl != NULL);
    ct_test(test, GEGL_IS_ADD_IMPL(add_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_ADD_IMPL) == GEGL_TYPE_POINT_OP_IMPL);
    ct_test(test, !strcmp("GeglAddImpl", g_type_name(GEGL_TYPE_ADD_IMPL)));

    g_object_unref(add_impl);
  }
}

static void
test_add_impl_get_num_inputs(Test *test)
{
  {
    GeglAddImpl * add_impl = g_object_new (GEGL_TYPE_ADD_IMPL, NULL);  

    ct_test(test, add_impl != NULL);
    ct_test(test, 2 == gegl_op_impl_get_num_inputs(GEGL_OP_IMPL(add_impl)));

    g_object_unref(add_impl);
  }
}

static void
add_impl_test_setup(Test *test)
{
}

static void
add_impl_test_teardown(Test *test)
{
}

Test *
create_add_impl_test()
{
  Test* t = ct_create("GeglAddImplTest");

  g_assert(ct_addSetUp(t, add_impl_test_setup));
  g_assert(ct_addTearDown(t, add_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_add_impl_g_object_new));
  g_assert(ct_addTestFun(t, test_add_impl_get_num_inputs));

  return t; 
}
