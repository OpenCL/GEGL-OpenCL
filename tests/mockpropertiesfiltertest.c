#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-properties-filter.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_properties_filter_g_object_new(Test *test)
{
  {
    GeglMockPropertiesFilter * mock_properties_filter = 
      g_object_new (GEGL_TYPE_MOCK_PROPERTIES_FILTER, NULL);  

    ct_test(test, mock_properties_filter != NULL);

    ct_test(test, GEGL_IS_MOCK_PROPERTIES_FILTER(mock_properties_filter));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_PROPERTIES_FILTER) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglMockPropertiesFilter", g_type_name(GEGL_TYPE_MOCK_PROPERTIES_FILTER)));
    ct_test(test, GEGL_IS_MOCK_PROPERTIES_FILTER(mock_properties_filter));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_PROPERTIES_FILTER) == GEGL_TYPE_FILTER);
    ct_test(test, !strcmp("GeglMockPropertiesFilter", g_type_name(GEGL_TYPE_MOCK_PROPERTIES_FILTER)));

    g_object_unref(mock_properties_filter);
  }
}

static void
test_mock_properties_filter_g_object_new_properties(Test *test)
{
  {
    GeglNode *a;
    a = g_object_new (GEGL_TYPE_MOCK_PROPERTIES_FILTER, 
                      "scalar-int", 1000, 
                      "scalar-float", .5, 
                      "channel-uint8", 255,
                      "pixel-rgb-float", 1.0, 1.0, 1.0,
                      NULL);  
    g_object_unref(a);
  }
}

static void
test_mock_properties_filter_g_object_set(Test *test)
{
  {
    GeglNode *a;
    a = g_object_new (GEGL_TYPE_MOCK_PROPERTIES_FILTER, NULL);  
    g_object_set(a, "scalar-int", 1000, NULL); 
    g_object_set(a, "scalar-float", .5, NULL); 
    g_object_set(a, "channel-uint8", 255, NULL); 
    g_object_set(a, "channel-float", .5, NULL); 
    g_object_set(a, "pixel-rgb-float", .5, .5, .5, NULL); 
    g_object_set(a, "pixel-rgba-float", .5, .5, .5, 1.0, NULL); 
    g_object_set(a, "pixel-rgb-uint8", 128, 128, 128, NULL); 
    g_object_set(a, "pixel-rgba-uint8", 128, 128, 128, 255, NULL); 
    g_object_unref(a);
  }
}

static void
mock_properties_filter_test_setup(Test *test)
{
}

static void
mock_properties_filter_test_teardown(Test *test)
{
}

Test *
create_mock_properties_filter_test()
{
  Test* t = ct_create("GeglMockPropertiesFilterTest");

  g_assert(ct_addSetUp(t, mock_properties_filter_test_setup));
  g_assert(ct_addTearDown(t, mock_properties_filter_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_mock_properties_filter_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_properties_filter_g_object_new_properties));
  g_assert(ct_addTestFun(t, test_mock_properties_filter_g_object_set));
#endif
                                     
  return t; 
}
