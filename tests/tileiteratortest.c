#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglTile * tile;
static GeglColorModel * color_model;
#define TILE_WIDTH 5 
#define TILE_HEIGHT 4 
static GeglRect tile_rect = {0,0,TILE_WIDTH,TILE_HEIGHT};

#define ITER_AREA_WIDTH 5 
#define ITER_AREA_HEIGHT 4 
static GeglRect iter_area = {0,0,ITER_AREA_WIDTH,ITER_AREA_HEIGHT};

static void
test_tile_iterator_g_object_new(Test *test)
{
  {
    GeglTileIterator * iterator = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                                "tile", tile,
                                                "area", &iter_area,
                                                NULL);  

    ct_test(test, iterator != NULL);
    ct_test(test, GEGL_IS_TILE_ITERATOR(iterator));
    ct_test(test, g_type_parent(GEGL_TYPE_TILE_ITERATOR) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglTileIterator", g_type_name(GEGL_TYPE_TILE_ITERATOR)));

    g_object_unref(iterator);
  }
}

static void
test_tile_iterator_get_num_colors(Test *test)
{
  {
    GeglTileIterator * iterator = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                                "tile", tile,
                                                "area", &iter_area,
                                                NULL);  
    gint num_colors = gegl_tile_iterator_get_num_colors(iterator);

    ct_test(test, 3 == num_colors);
    g_object_unref(iterator);
  }
}

static void
test_tile_iterator_current_alpha_ptr(Test *test)
{
  {
    GeglTileIterator * iterator = g_object_new (GEGL_TYPE_TILE_ITERATOR, 
                                                "tile", tile,
                                                "area", &iter_area,
                                                NULL);  
    gpointer alpha_ptr = gegl_tile_iterator_alpha_channel(iterator);

    ct_test(test, NULL == alpha_ptr);
    g_object_unref(iterator);
  }
}

static void
tile_iterator_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
  tile = g_object_new (GEGL_TYPE_TILE, 
                       "area", &tile_rect, 
                       "colormodel", color_model,
                       NULL);  
}

static void
tile_iterator_test_teardown(Test *test)
{
  g_object_unref(tile);
}

Test *
create_tile_iterator_test()
{
  Test* t = ct_create("GeglTileIteratorTest");

  g_assert(ct_addSetUp(t, tile_iterator_test_setup));
  g_assert(ct_addTearDown(t, tile_iterator_test_teardown));
  g_assert(ct_addTestFun(t, test_tile_iterator_g_object_new));
  g_assert(ct_addTestFun(t, test_tile_iterator_get_num_colors));
  g_assert(ct_addTestFun(t, test_tile_iterator_current_alpha_ptr));

  return t; 
}
