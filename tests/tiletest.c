#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglColorModel * color_model;
#define AREA_WIDTH 5 
#define AREA_HEIGHT 4 
static GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};

static void
test_tile_g_object_new(Test *test)
{
  GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  

  ct_test(test, tile != NULL);
  ct_test(test, GEGL_IS_TILE(tile));
  ct_test(test, g_type_parent(GEGL_TYPE_TILE) == GEGL_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglTile", g_type_name(GEGL_TYPE_TILE)));

  g_object_unref(tile);
}

static void
test_tile_get(Test *test)
{
  GeglBuffer *buffer;
  GeglRect a;
  GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  

  gegl_tile_get_area(tile, &a);
  ct_test(test, gegl_rect_equal(&a, &area));

  ct_test(test, color_model == gegl_tile_get_color_model(tile));

  buffer = gegl_tile_get_buffer(tile);
  ct_test(test, NULL != buffer);
  ct_test(test, 4 * AREA_WIDTH * AREA_HEIGHT == gegl_buffer_get_bytes_per_buffer(buffer));
  ct_test(test, 3 == gegl_buffer_get_num_buffers(buffer));

  g_object_unref(tile);
}

static void
tile_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
}

static void
tile_test_teardown(Test *test)
{
  g_object_unref(color_model);
}

Test *
create_tile_test()
{
  Test* t = ct_create("GeglTileTest");

  g_assert(ct_addSetUp(t, tile_test_setup));
  g_assert(ct_addTearDown(t, tile_test_teardown));
  g_assert(ct_addTestFun(t, test_tile_g_object_new));
  g_assert(ct_addTestFun(t, test_tile_get));

  return t; 
}
