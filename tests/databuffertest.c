#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_data_buffer_g_object_new(Test *test)
{
  {
    GeglDataBuffer * data_buffer = g_object_new (GEGL_TYPE_DATA_BUFFER, NULL);  

    ct_test(test, data_buffer != NULL);
    ct_test(test, GEGL_IS_DATA_BUFFER(data_buffer));
    ct_test(test, g_type_parent(GEGL_TYPE_DATA_BUFFER) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglDataBuffer", g_type_name(GEGL_TYPE_DATA_BUFFER)));

    g_object_unref(data_buffer);
  }
}

static void
test_data_buffer_properties(Test *test)
{
  {
    GeglDataBuffer * data_buffer = g_object_new(GEGL_TYPE_DATA_BUFFER,
                                                "bytes_per_bank", 4,
                                                "num_banks", 3, 
                                                NULL);

    ct_test(test, 3 == gegl_data_buffer_num_banks(data_buffer));
    ct_test(test, 4 == gegl_data_buffer_bytes_per_bank(data_buffer));
    ct_test(test, 12 == gegl_data_buffer_total_bytes(data_buffer));
    ct_test(test, NULL != gegl_data_buffer_banks_data(data_buffer));

    g_object_unref(data_buffer);
  }
}

static void
data_buffer_test_setup(Test *test)
{
}

static void
data_buffer_test_teardown(Test *test)
{
}

Test *
create_data_buffer_test()
{
  Test* t = ct_create("GeglDataBufferTest");

  g_assert(ct_addSetUp(t, data_buffer_test_setup));
  g_assert(ct_addTearDown(t, data_buffer_test_teardown));
  g_assert(ct_addTestFun(t, test_data_buffer_g_object_new));
  g_assert(ct_addTestFun(t, test_data_buffer_properties));

  return t; 
}
