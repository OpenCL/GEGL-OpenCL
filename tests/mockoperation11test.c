#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter-1-1.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_filter_1_1_g_object_new(Test *test)
{
  {
    GeglNode * mock_filter_1_1 = g_object_new (GEGL_TYPE_MOCK_FILTER_1_1, NULL);

    ct_test(test, GEGL_IS_MOCK_FILTER_1_1(mock_filter_1_1));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_FILTER_1_1) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglMockFilter11", g_type_name(GEGL_TYPE_MOCK_FILTER_1_1)));

    g_object_unref(mock_filter_1_1);
  }
}

static void
test_mock_filter_1_1_g_object_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_1, NULL);
    gint i;

    /* Initially the default input is 100 */
    g_object_get(a, "input0", &i, NULL);
    ct_test(test, i == 100);

    gegl_node_apply(a, "output0");

    /* So the first output should be 2 * 100 */
    g_object_get(a, "output0", &i, NULL);
    ct_test(test, i == 200);

    /* Change the input to 2 */
    g_object_set(a, "input0", 2, NULL);
    g_object_get(a, "input0", &i, NULL);

    ct_test(test, i == 2);

    gegl_node_apply(a, "output0");

    /* Now the new output should be 2 * 2 */
    g_object_get(a, "output0", &i, NULL);
    ct_test(test, i == 4);

    g_object_unref(a);
  }
}

static void
test_mock_filter_1_1_num_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_1, NULL);

    ct_test(test, 1 == gegl_node_get_num_input_props(a));
    ct_test(test, 1 == gegl_node_get_num_output_props(a));

    g_object_unref(a);
  }
}

static void
test_mock_filter_1_1_property_names(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_1, NULL);
    GeglProperty *output0 = gegl_node_get_property(a, "output0");
    GeglProperty *input0 = gegl_node_get_property(a, "input0");

    ct_test(test, output0 != NULL);
    ct_test(test, input0 != NULL);

    g_object_unref(a);
  }
}

static void
mock_filter_1_1_test_setup(Test *test)
{
}

static void
mock_filter_1_1_test_teardown(Test *test)
{
}

Test *
create_mock_filter_1_1_test()
{
  Test* t = ct_create("GeglMockFilter11Test");

  g_assert(ct_addSetUp(t, mock_filter_1_1_test_setup));
  g_assert(ct_addTearDown(t, mock_filter_1_1_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_mock_filter_1_1_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_filter_1_1_g_object_properties));
  g_assert(ct_addTestFun(t, test_mock_filter_1_1_num_properties));
  g_assert(ct_addTestFun(t, test_mock_filter_1_1_property_names));
#endif

  return t;
}
