#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_storage_g_object_new(Test *test)
{
  {
    GeglStorage * storage = g_object_new (GEGL_TYPE_COMPONENT_STORAGE, NULL);  

    ct_test(test, storage != NULL);
    ct_test(test, GEGL_IS_STORAGE(storage));
    ct_test(test, GEGL_IS_COMPONENT_STORAGE(storage));
    ct_test(test, g_type_parent(GEGL_TYPE_STORAGE) == GEGL_TYPE_OBJECT);
    ct_test(test, g_type_parent(GEGL_TYPE_COMPONENT_STORAGE) == GEGL_TYPE_STORAGE);
    ct_test(test, !strcmp("GeglStorage", g_type_name(GEGL_TYPE_STORAGE)));
    ct_test(test, !strcmp("GeglComponentStorage", g_type_name(GEGL_TYPE_COMPONENT_STORAGE)));

    g_object_unref(storage);
  }
}

static void
test_storage_component_storage_properties(Test *test)
{
  {
    GeglStorage * storage = g_object_new(GEGL_TYPE_COMPONENT_STORAGE,
                                         "data_type_bytes", 4,
                                         "num_bands", 3,  
                                         "width", 10,
                                         "height", 10,
                                         "num_banks", 3, 
                                         NULL);

    ct_test(test, 4 == gegl_storage_data_type_bytes(storage));
    ct_test(test, 3 == gegl_storage_num_bands(storage));
    ct_test(test, 10 == gegl_storage_width(storage));
    ct_test(test, 10 == gegl_storage_height(storage));

    ct_test(test, 3 == gegl_component_storage_num_banks(GEGL_COMPONENT_STORAGE(storage)));
    g_object_unref(storage);
  }
}

static void
test_storage_component_create_data_buffer(Test *test)
{
  {
    GeglStorage * storage = g_object_new(GEGL_TYPE_COMPONENT_STORAGE,
                                         "data_type_bytes", 4,
                                         "num_bands", 3,  
                                         "width", 2,
                                         "height", 2,
                                         "num_banks", 3, 
                                         NULL);

    GeglDataBuffer *data_buffer = gegl_storage_create_data_buffer(storage);

    ct_test(test, 3 == gegl_data_buffer_num_banks(data_buffer));
    ct_test(test, 16 == gegl_data_buffer_bytes_per_bank(data_buffer));
    ct_test(test, 48 == gegl_data_buffer_total_bytes(data_buffer));
    ct_test(test, NULL != gegl_data_buffer_banks_data(data_buffer));

    g_object_unref(storage);
    g_object_unref(data_buffer);
  }
}

static void
storage_test_setup(Test *test)
{
}

static void
storage_test_teardown(Test *test)
{
}

Test *
create_storage_test()
{
  Test* t = ct_create("GeglStorageTest");

  g_assert(ct_addSetUp(t, storage_test_setup));
  g_assert(ct_addTearDown(t, storage_test_teardown));
  g_assert(ct_addTestFun(t, test_storage_g_object_new));
  g_assert(ct_addTestFun(t, test_storage_component_storage_properties));
  g_assert(ct_addTestFun(t, test_storage_component_create_data_buffer));

  return t; 
}
