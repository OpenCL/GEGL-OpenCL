#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_pipe_g_object_new(Test *test)
{
  {
    GeglPipe * pipe = g_object_new (GEGL_TYPE_PIPE, NULL);  

    ct_test(test, pipe != NULL);
    ct_test(test, GEGL_IS_PIPE(pipe));
    ct_test(test, g_type_parent(GEGL_TYPE_PIPE) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglPipe", g_type_name(GEGL_TYPE_PIPE)));

    g_object_unref(pipe);
  }
}

static void
pipe_test_setup(Test *test)
{
}

static void
pipe_test_teardown(Test *test)
{
}

Test *
create_pipe_test()
{
  Test* t = ct_create("GeglPipeTest");

  g_assert(ct_addSetUp(t, pipe_test_setup));
  g_assert(ct_addTearDown(t, pipe_test_teardown));
  g_assert(ct_addTestFun(t, test_pipe_g_object_new));

  return t; 
}
