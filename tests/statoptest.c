#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_stat_op_g_object_new(Test *test)
{
  {
    GeglStatOp * stat_op = g_object_new (GEGL_TYPE_STAT_OP, NULL);  

    ct_test(test, stat_op != NULL);
    ct_test(test, GEGL_IS_STAT_OP(stat_op));
    ct_test(test, g_type_parent(GEGL_TYPE_STAT_OP) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglStatOp", g_type_name(GEGL_TYPE_STAT_OP)));

    g_object_unref(stat_op);
  }
}

static void
stat_op_test_setup(Test *test)
{
}

static void
stat_op_test_teardown(Test *test)
{
}

Test *
create_stat_op_test()
{
  Test* t = ct_create("GeglStatOpTest");

  g_assert(ct_addSetUp(t, stat_op_test_setup));
  g_assert(ct_addTearDown(t, stat_op_test_teardown));
  g_assert(ct_addTestFun(t, test_stat_op_g_object_new));

  return t; 
}
