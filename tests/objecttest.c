#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-object.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_object_g_object_new(Test *test)
{
  GeglObject *object = g_object_new (GEGL_TYPE_MOCK_OBJECT, NULL);  

  ct_test(test, object  != NULL);
  ct_test(test, GEGL_IS_OBJECT(object));
  ct_test(test, g_type_parent(GEGL_TYPE_OBJECT) == G_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglObject", g_type_name(GEGL_TYPE_OBJECT)));

  ct_test(test, GEGL_IS_MOCK_OBJECT(object));
  ct_test(test, g_type_parent(GEGL_TYPE_MOCK_OBJECT) == GEGL_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglMockObject", g_type_name(GEGL_TYPE_MOCK_OBJECT)));

  g_object_unref(object);
}

static void
test_object_g_object_new_name(Test *test)
{
  gchar * name = "hello";
  GeglObject *object = g_object_new (GEGL_TYPE_MOCK_OBJECT, 
                                     "name", name, 
                                     NULL);  

  ct_test(test, !strcmp("hello", gegl_object_get_name(object)));
  g_object_unref(object);
}

static void
object_test_setup(Test *test)
{
}

static void
object_test_teardown(Test *test)
{
}

Test *
create_object_test()
{
  Test* t = ct_create("GeglObjectTest");

  g_assert(ct_addSetUp(t, object_test_setup));
  g_assert(ct_addTearDown(t, object_test_teardown));
  g_assert(ct_addTestFun(t, test_object_g_object_new));
  g_assert(ct_addTestFun(t, test_object_g_object_new_name));

  return t; 
}
