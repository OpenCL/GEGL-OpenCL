#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter-1-2.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_filter_1_2_g_object_new(Test *test)
{
  {
    GeglNode * mock_filter_1_2 = g_object_new (GEGL_TYPE_MOCK_FILTER_1_2, NULL);

    ct_test(test, GEGL_IS_MOCK_FILTER_1_2(mock_filter_1_2));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_FILTER_1_2) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglMockFilter12", g_type_name(GEGL_TYPE_MOCK_FILTER_1_2)));

    g_object_unref(mock_filter_1_2);
  }
}

static void
test_mock_filter_1_2_g_object_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_2, NULL);
    gint i;

    g_object_get(a, "output0", &i, NULL);
    ct_test(test, i == 0);

    g_object_get(a, "output1", &i, NULL);
    ct_test(test, i == 0);

    g_object_get(a, "input0", &i, NULL);
    ct_test(test, i == 500);

    g_object_set(a, "input0", 750, NULL);

    g_object_get(a, "input0", &i, NULL);
    ct_test(test, i == 750);

    g_object_unref(a);
  }
}

static void
test_mock_filter_1_2_num_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_2, NULL);

    ct_test(test, 1 == gegl_node_get_num_input_props(a));
    ct_test(test, 2 == gegl_node_get_num_output_props(a));

    g_object_unref(a);
  }
}

static void
test_mock_filter_1_2_property_names(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_FILTER_1_2, NULL);
    GeglProperty *output0 = gegl_node_get_property(a, "output0");
    GeglProperty *output1 = gegl_node_get_property(a, "output1");
    GeglProperty *input0 = gegl_node_get_property(a, "input0");

    ct_test(test, output0 != NULL);
    ct_test(test, output1 != NULL);
    ct_test(test, input0 != NULL);

    g_object_unref(a);
  }
}

static void
mock_filter_1_2_test_setup(Test *test)
{
}

static void
mock_filter_1_2_test_teardown(Test *test)
{
}

Test *
create_mock_filter_1_2_test()
{
  Test* t = ct_create("GeglMockFilter12Test");

  g_assert(ct_addSetUp(t, mock_filter_1_2_test_setup));
  g_assert(ct_addTearDown(t, mock_filter_1_2_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_mock_filter_1_2_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_filter_1_2_g_object_properties));
  g_assert(ct_addTestFun(t, test_mock_filter_1_2_num_properties));
  g_assert(ct_addTestFun(t, test_mock_filter_1_2_property_names));
#endif

  return t;
}
