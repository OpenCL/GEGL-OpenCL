#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static GeglStorage *storage;

static void
test_tile_g_object_new(Test *test)
{
  {
    GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                        "x", 0, 
                                        "y", 0, 
                                        "width", 2, 
                                        "height", 2,
                                        "storage", storage,
                                        NULL);  

    ct_test(test, tile != NULL);
    ct_test(test, GEGL_IS_TILE(tile));
    ct_test(test, g_type_parent(GEGL_TYPE_TILE) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglTile", g_type_name(GEGL_TYPE_TILE)));

    g_object_unref(tile);
  }
}

static void
test_tile_properties(Test *test)
{
  {
    gpointer *data_pointers = NULL;
    GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                        "x", 0, 
                                        "y", 0, 
                                        "width", 2, 
                                        "height", 2,
                                        "storage", storage,
                                        NULL);  

    ct_test(test, 0 == gegl_tile_get_x(tile));
    ct_test(test, 0 == gegl_tile_get_y(tile));
    ct_test(test, 2 == gegl_tile_get_width(tile));
    ct_test(test, 2 == gegl_tile_get_height(tile));
    ct_test(test, storage == gegl_tile_get_storage(tile));
    ct_test(test, NULL != gegl_tile_get_data_buffer(tile));
    data_pointers = gegl_tile_data_pointers(tile,0,0);
    ct_test(test, NULL != data_pointers);
    ct_test(test, NULL != data_pointers[0]);
    ct_test(test, NULL != data_pointers[1]);
    ct_test(test, NULL != data_pointers[2]);
    g_free(data_pointers);
    data_pointers = gegl_tile_data_pointers(tile,1,1);
    ct_test(test, NULL != data_pointers);
    ct_test(test, NULL != data_pointers[0]);
    ct_test(test, NULL != data_pointers[1]);
    ct_test(test, NULL != data_pointers[2]);
    g_free(data_pointers);

    g_object_unref(tile);
  }
}

static void
tile_test_setup(Test *test)
{
  storage = g_object_new(GEGL_TYPE_COMPONENT_STORAGE,
                         "data_type_bytes", 4,
                         "num_bands", 3,  
                         "width", 2,
                         "height", 2,
                         "num_banks", 3, 
                         NULL);
}

static void
tile_test_teardown(Test *test)
{
  g_object_unref(storage);
}

Test *
create_tile_test()
{
  Test* t = ct_create("GeglTileTest");

  g_assert(ct_addSetUp(t, tile_test_setup));
  g_assert(ct_addTearDown(t, tile_test_teardown));
  g_assert(ct_addTestFun(t, test_tile_g_object_new));
  g_assert(ct_addTestFun(t, test_tile_properties));

  return t; 
}
