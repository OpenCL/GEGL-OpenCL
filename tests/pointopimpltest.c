#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_point_op_impl_g_object_new(Test *test)
{
  {
    GeglPointOpImpl * point_op_impl = g_object_new (GEGL_TYPE_POINT_OP_IMPL, NULL);  

    ct_test(test, point_op_impl != NULL);
    ct_test(test, GEGL_IS_POINT_OP_IMPL(point_op_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_POINT_OP_IMPL) == GEGL_TYPE_IMAGE_IMPL);
    ct_test(test, !strcmp("GeglPointOpImpl", g_type_name(GEGL_TYPE_POINT_OP_IMPL)));

    g_object_unref(point_op_impl);
  }
}

static void
point_op_impl_test_setup(Test *test)
{
}

static void
point_op_impl_test_teardown(Test *test)
{
}

Test *
create_point_op_impl_test()
{
  Test* t = ct_create("GeglPointOpImplTest");

  g_assert(ct_addSetUp(t, point_op_impl_test_setup));
  g_assert(ct_addTearDown(t, point_op_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_point_op_impl_g_object_new));

  return t; 
}
