#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_image_mgr_g_object_new(Test *test)
{
  GeglImageMgr * image_mgr = gegl_image_mgr_instance();  

  ct_test(test, NULL != image_mgr);
  ct_test(test, GEGL_IS_IMAGE_MGR(image_mgr));
  ct_test(test, g_type_parent(GEGL_TYPE_IMAGE_MGR) == GEGL_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglImageMgr", g_type_name(GEGL_TYPE_IMAGE_MGR)));

  g_object_unref(image_mgr);
}

static void
image_mgr_test_setup(Test *test)
{
}

static void
image_mgr_test_teardown(Test *test)
{
}

Test *
create_image_mgr_test()
{
  Test* t = ct_create("GeglImageMgrTest");

  g_assert(ct_addSetUp(t, image_mgr_test_setup));
  g_assert(ct_addTearDown(t, image_mgr_test_teardown));
  g_assert(ct_addTestFun(t, test_image_mgr_g_object_new));

  return t; 
}
