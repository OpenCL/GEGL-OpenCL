#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_buffer_g_object_new(Test *test)
{
  {
    GeglBuffer * buffer = g_object_new (GEGL_TYPE_BUFFER, NULL);  

    ct_test(test, buffer != NULL);
    ct_test(test, GEGL_IS_BUFFER(buffer));
    ct_test(test, g_type_parent(GEGL_TYPE_BUFFER) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglBuffer", g_type_name(GEGL_TYPE_BUFFER)));

    g_object_unref(buffer);
  }
}

static void
test_buffer_properties(Test *test)
{
  {
    GeglBuffer * buffer = g_object_new(GEGL_TYPE_BUFFER,
                                                "bytes_per_bank", 4,
                                                "num_banks", 3, 
                                                NULL);

    ct_test(test, 3 == gegl_buffer_num_banks(buffer));
    ct_test(test, 4 == gegl_buffer_bytes_per_bank(buffer));
    ct_test(test, 12 == gegl_buffer_total_bytes(buffer));
    ct_test(test, NULL != gegl_buffer_banks_data(buffer));

    g_object_unref(buffer);
  }
}

static void
buffer_test_setup(Test *test)
{
}

static void
buffer_test_teardown(Test *test)
{
}

Test *
create_buffer_test()
{
  Test* t = ct_create("GeglBufferTest");

  g_assert(ct_addSetUp(t, buffer_test_setup));
  g_assert(ct_addTearDown(t, buffer_test_teardown));
  g_assert(ct_addTestFun(t, test_buffer_g_object_new));
  g_assert(ct_addTestFun(t, test_buffer_properties));

  return t; 
}
