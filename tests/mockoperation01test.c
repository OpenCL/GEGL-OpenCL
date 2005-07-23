#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-operation-0-1.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_operation_0_1_g_object_new(Test *test)
{
  {
    GeglNode * mock_operation_0_1 = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, NULL);

    ct_test(test, GEGL_IS_MOCK_OPERATION_0_1(mock_operation_0_1));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_OPERATION_0_1) == GEGL_TYPE_OPERATION);
    ct_test(test, !strcmp("GeglMockOperation01", g_type_name(GEGL_TYPE_MOCK_OPERATION_0_1)));

    g_object_unref(mock_operation_0_1);
  }
}

static void
test_mock_operation_0_1_g_object_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, NULL);
    gint i;

    gegl_node_apply(a, "output0");
    g_object_get(a, "output0", &i, NULL);

    ct_test(test, i == 1);

    g_object_unref(a);
  }
}

static void
test_mock_operation_0_1_num_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, NULL);

    ct_test(test, 0 == gegl_node_get_num_input_pads(a));
    ct_test(test, 1 == gegl_node_get_num_output_pads(a));

    g_object_unref(a);
  }
}

static void
test_mock_operation_0_1_pad_names(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_OPERATION_0_1, NULL);
    GeglPad *output0 = gegl_node_get_pad(a, "output0");

    ct_test(test, output0 != NULL);

    g_object_unref(a);
  }
}

static void
mock_operation_0_1_test_setup(Test *test)
{
}

static void
mock_operation_0_1_test_teardown(Test *test)
{
}

Test *
create_mock_operation_0_1_test()
{
  Test* t = ct_create("GeglMockOperation01Test");

  g_assert(ct_addSetUp(t, mock_operation_0_1_test_setup));
  g_assert(ct_addTearDown(t, mock_operation_0_1_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_mock_operation_0_1_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_operation_0_1_g_object_properties));
  g_assert(ct_addTestFun(t, test_mock_operation_0_1_num_properties));
  g_assert(ct_addTestFun(t, test_mock_operation_0_1_pad_names));
#endif

  return t;
}
