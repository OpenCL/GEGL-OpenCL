#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static void
test_tile_mgr_g_object_new(Test *test)
{
  GeglTileMgr * tile_mgr = g_object_new(GEGL_TYPE_TILE_MGR,NULL); 

  ct_test(test, tile_mgr != NULL);
  ct_test(test, GEGL_IS_TILE_MGR(tile_mgr));
  ct_test(test, g_type_parent(GEGL_TYPE_TILE_MGR) == GEGL_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglTileMgr", g_type_name(GEGL_TYPE_TILE_MGR)));

  g_object_unref(tile_mgr);
}

static void
tile_mgr_test_setup(Test *test)
{
}

static void
tile_mgr_test_teardown(Test *test)
{
}

Test *
create_tile_mgr_test()
{
  Test* t = ct_create("GeglTileMgrTest");

  g_assert(ct_addSetUp(t, tile_mgr_test_setup));
  g_assert(ct_addTearDown(t, tile_mgr_test_teardown));
  g_assert(ct_addTestFun(t, test_tile_mgr_g_object_new));

  return t; 
}
