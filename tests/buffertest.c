#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define BYTES_PER_BUFFER 10 
#define NUM_BUFFERS 3 

static void
test_buffer_g_object_new(Test *test)
{
  {
    GeglBuffer * buffer = g_object_new (GEGL_TYPE_BUFFER, NULL);  

    ct_test(test, buffer  != NULL);
    ct_test(test, GEGL_IS_BUFFER(buffer));
    ct_test(test, g_type_parent(GEGL_TYPE_OBJECT) == G_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglBuffer", g_type_name(GEGL_TYPE_BUFFER)));

    ct_test(test, 0 == gegl_buffer_get_bytes_per_buffer(buffer));
    ct_test(test, 0 == gegl_buffer_get_num_buffers(buffer));

    g_object_unref(buffer);
  }

  {
    GeglBuffer *buffer = g_object_new (GEGL_TYPE_BUFFER, 
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
test_buffer_g_object_get(Test *test)
{
  gint bytes_per_buffer;
  gint num_buffers;

  GeglBuffer *buffer = g_object_new (GEGL_TYPE_BUFFER, 
                                     "bytesperbuffer", BYTES_PER_BUFFER, 
                                     "numbuffers", NUM_BUFFERS,
                                      NULL);  

  g_object_get(G_OBJECT(buffer), 
               "bytesperbuffer", &bytes_per_buffer,
               "numbuffers", &num_buffers, 
               NULL); 

  ct_test(test, BYTES_PER_BUFFER == bytes_per_buffer);
  ct_test(test, NUM_BUFFERS == num_buffers);

  g_object_unref(buffer);
}

static void
test_buffer_g_object_set(Test *test)
{
  GeglBuffer *buffer = g_object_new (GEGL_TYPE_BUFFER, NULL);  
 
  /*
  This should give a warning: bytesperbuffer, numbuffers writable
  at construction only. 

  g_object_set(G_OBJECT(buffer), 
               "bytesperbuffer", BYTES_PER_BUFFER, 
               "numbuffers", NUM_BUFFERS, 
               NULL); 

  ct_test(test, BYTES_PER_BUFFER == gegl_buffer_get_bytes_per_buffer(buffer));
  ct_test(test, NUM_BUFFERS == gegl_buffer_get_num_buffers(buffer));
  */

  g_object_unref(buffer);
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
  g_assert(ct_addTestFun(t, test_buffer_g_object_get));
  g_assert(ct_addTestFun(t, test_buffer_g_object_set));

  return t; 
}
