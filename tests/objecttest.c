#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-object.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

GeglObject * object;

static void
test_object_g_object_new(Test *test)
{
  ct_test(test, object  != NULL);
  ct_test(test, GEGL_IS_OBJECT(object));
  ct_test(test, g_type_parent(GEGL_TYPE_OBJECT) == G_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglObject", g_type_name(GEGL_TYPE_OBJECT)));

  ct_test(test, GEGL_IS_MOCK_OBJECT(object));
  ct_test(test, g_type_parent(GEGL_TYPE_MOCK_OBJECT) == GEGL_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglMockObject", g_type_name(GEGL_TYPE_MOCK_OBJECT)));
}

static void
test_object_add_interface(Test *test)
{
  gint some_int;

  gegl_object_add_interface(object, "some_interface", (gpointer)&some_int);
  ct_test(test, gegl_object_query_interface(object, "some_interface") == (gpointer)&some_int);
}

static void
object_test_setup(Test *test)
{
  object = g_object_new (GEGL_TYPE_MOCK_OBJECT, NULL);  
}

static void
object_test_teardown(Test *test)
{
  g_object_unref(G_OBJECT(object));
}

Test *
create_object_test()
{
  Test* t = ct_create("GeglObjectTest");

  g_assert(ct_addSetUp(t, object_test_setup));
  g_assert(ct_addTearDown(t, object_test_teardown));
  g_assert(ct_addTestFun(t, test_object_g_object_new));
  g_assert(ct_addTestFun(t, test_object_add_interface));

  return t; 
}
