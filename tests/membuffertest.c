#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define BYTES_PER_BUFFER 10 
#define NUM_BUFFERS 3 

static void
test_mem_buffer_g_object_new(Test *test)
{
  {
    GeglBuffer * buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, NULL);  

    ct_test(test, buffer  != NULL);
    ct_test(test, GEGL_IS_MEM_BUFFER(buffer));
    ct_test(test, g_type_parent(GEGL_TYPE_MEM_BUFFER) == GEGL_TYPE_BUFFER);
    ct_test(test, !strcmp("GeglMemBuffer", g_type_name(GEGL_TYPE_MEM_BUFFER)));

    g_object_unref(buffer);
  }

  {
    GeglBuffer *buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, 
                                       "bytesperbuffer", BYTES_PER_BUFFER, 
                                       "numbuffers", NUM_BUFFERS,
                                        NULL);  

    ct_test(test, buffer  != NULL);

    ct_test(test, BYTES_PER_BUFFER == gegl_buffer_get_bytes_per_buffer(buffer));
    ct_test(test, NUM_BUFFERS == gegl_buffer_get_num_buffers(buffer));

    g_object_unref(buffer);
  }
}

static void
test_mem_buffer_g_object_get(Test *test)
{
  gint bytes_per_buffer;
  gint num_buffers;

  GeglBuffer *buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, 
                                     "bytesperbuffer", BYTES_PER_BUFFER, 
                                     "numbuffers", NUM_BUFFERS,
                                      NULL);  

  ct_test(test, buffer  != NULL);

  g_object_get(buffer, 
               "bytesperbuffer", &bytes_per_buffer,
               "numbuffers", &num_buffers, 
               NULL); 

  ct_test(test, BYTES_PER_BUFFER == bytes_per_buffer);
  ct_test(test, NUM_BUFFERS == num_buffers);

  g_object_unref(buffer);
}

static void
test_mem_buffer_get (Test *test)
{
  GeglBuffer *buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, 
                                     "bytesperbuffer", BYTES_PER_BUFFER, 
                                     "numbuffers", NUM_BUFFERS,
                                      NULL);  

  ct_test(test, buffer  != NULL);
  ct_test(test, BYTES_PER_BUFFER == gegl_buffer_get_bytes_per_buffer(buffer));
  ct_test(test, NUM_BUFFERS == gegl_buffer_get_num_buffers(buffer));
  ct_test(test, NULL != gegl_buffer_get_data_pointers(buffer));

  g_object_unref(buffer);
}

static void
test_mem_buffer_unalloc(Test *test)
{
  GeglBuffer *buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, 
                                     "bytesperbuffer", BYTES_PER_BUFFER, 
                                     "numbuffers", NUM_BUFFERS,
                                      NULL);  

  ct_test(test, NULL != gegl_buffer_get_data_pointers(buffer));
  gegl_buffer_unalloc_data(buffer); 
  ct_test(test, NULL == gegl_buffer_get_data_pointers(buffer));
  gegl_buffer_alloc_data(buffer); 
  ct_test(test, NULL != gegl_buffer_get_data_pointers(buffer));

  g_object_unref(buffer);
}

static void
test_mem_buffer_ref_unref(Test *test)
{
  GeglBuffer *buffer = g_object_new (GEGL_TYPE_MEM_BUFFER, 
                                     "bytesperbuffer", BYTES_PER_BUFFER, 
                                     "numbuffers", NUM_BUFFERS,
                                      NULL);  

  gegl_buffer_ref(buffer);
  ct_test(test, 1 == gegl_buffer_get_ref_count(buffer));
  gegl_buffer_ref(buffer);
  ct_test(test, 2 == gegl_buffer_get_ref_count(buffer));
  gegl_buffer_unref(buffer);
  ct_test(test, 1 == gegl_buffer_get_ref_count(buffer));
  gegl_buffer_unref(buffer);
  ct_test(test, 0 == gegl_buffer_get_ref_count(buffer));
  gegl_buffer_unref(buffer);
  ct_test(test, 0 == gegl_buffer_get_ref_count(buffer));

  g_object_unref(buffer);
}

static void
mem_buffer_test_setup(Test *test)
{
}

static void
mem_buffer_test_teardown(Test *test)
{
}

Test *
create_mem_buffer_test()
{
  Test* t = ct_create("GeglMemBufferTest");

  g_assert(ct_addSetUp(t, mem_buffer_test_setup));
  g_assert(ct_addTearDown(t, mem_buffer_test_teardown));
  g_assert(ct_addTestFun(t, test_mem_buffer_g_object_new));
  g_assert(ct_addTestFun(t, test_mem_buffer_g_object_get));
  g_assert(ct_addTestFun(t, test_mem_buffer_get));
  g_assert(ct_addTestFun(t, test_mem_buffer_unalloc));
  g_assert(ct_addTestFun(t, test_mem_buffer_ref_unref));

  return t; 
}
