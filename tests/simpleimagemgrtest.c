#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_simple_image_mgr_g_object_new(Test *test)
{
  GeglImageMgr * image_mgr = g_object_new(GEGL_TYPE_SIMPLE_IMAGE_MGR,NULL);  

  ct_test(test, image_mgr != NULL);
  ct_test(test, GEGL_IS_SIMPLE_IMAGE_MGR(image_mgr));
  ct_test(test, g_type_parent(GEGL_TYPE_SIMPLE_IMAGE_MGR) == GEGL_TYPE_IMAGE_MGR);
  ct_test(test, !strcmp("GeglSimpleImageMgr", g_type_name(GEGL_TYPE_SIMPLE_IMAGE_MGR)));

  g_object_unref(image_mgr);
}

static void
test_simple_image_mgr_instance(Test *test)
{
  GeglImageMgr * image_mgr = gegl_image_mgr_instance();  

  ct_test(test, image_mgr != NULL);
  ct_test(test, GEGL_IS_SIMPLE_IMAGE_MGR(image_mgr));
  ct_test(test, g_type_parent(GEGL_TYPE_SIMPLE_IMAGE_MGR) == GEGL_TYPE_IMAGE_MGR);
  ct_test(test, !strcmp("GeglSimpleImageMgr", g_type_name(GEGL_TYPE_SIMPLE_IMAGE_MGR)));

  g_object_unref(image_mgr);
}

static void
simple_image_mgr_test_setup(Test *test)
{
}

static void
simple_image_mgr_test_teardown(Test *test)
{
}

Test *
create_simple_image_mgr_test()
{
  Test* t = ct_create("GeglSimpleImageMgrTest");

  g_assert(ct_addSetUp(t, simple_image_mgr_test_setup));
  g_assert(ct_addTearDown(t, simple_image_mgr_test_teardown));
  g_assert(ct_addTestFun(t, test_simple_image_mgr_g_object_new));
  g_assert(ct_addTestFun(t, test_simple_image_mgr_instance));

  return t; 
}
