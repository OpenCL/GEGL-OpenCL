#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglColorModel * color_model;
static GeglTile *tile;
#define AREA_WIDTH 10 
#define AREA_HEIGHT 11 
static GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
static GValue *value;

static void
test_value_new(Test *test)
{
  g_value_init(value, GEGL_TYPE_IMAGE_DATA);
  g_value_unset(value);
}

static void
test_value_set_image_data(Test *test)
{
  g_value_init(value, GEGL_TYPE_IMAGE_DATA);

  {
    GeglRect rect;
    gegl_rect_set(&rect, 1,2,3,4);
    g_value_set_image_data(value, tile, &rect); 
  }

  {
    GeglRect rect0;
    GeglTile * tile0 = g_value_get_image_data(value, &rect0);

    ct_test(test, tile == tile0); 
    ct_test(test, 1 == rect0.x); 
    ct_test(test, 2 == rect0.y); 
    ct_test(test, 3 == rect0.w); 
    ct_test(test, 4 == rect0.h); 
  }

  g_value_unset(value);
}

static void
test_value_set_image_data_rect(Test *test)
{
  g_value_init(value, GEGL_TYPE_IMAGE_DATA);

  {
    GeglRect rect;
    gegl_rect_set(&rect, 1,2,3,4);
    g_value_set_image_data_rect(value, &rect); 
  }

  {
    GeglRect rect0;
    g_value_get_image_data_rect(value, &rect0);

    ct_test(test, 1 == rect0.x); 
    ct_test(test, 2 == rect0.y); 
    ct_test(test, 3 == rect0.w); 
    ct_test(test, 4 == rect0.h); 
  }

  g_value_unset(value);
}

static void
test_value_set_image_data_tile(Test *test)
{
  g_value_init(value, GEGL_TYPE_IMAGE_DATA);

  {
    g_value_set_image_data_tile(value, tile); 
  }

  {
    GeglTile *tile0 = g_value_get_image_data_tile(value);
    ct_test(test, tile == tile0); 
  }

  g_value_unset(value);
}

static void
value_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
  tile = g_object_new (GEGL_TYPE_TILE, 
                       "area", &area, 
                       "colormodel", color_model,
                        NULL);  

  value =  g_new0(GValue, 1); 
}

static void
value_test_teardown(Test *test)
{
  g_object_unref(color_model);
  g_object_unref(tile);
  g_free(value);
}

Test *
create_value_test()
{
  Test* t = ct_create("GeglValueTest");

  g_assert(ct_addSetUp(t, value_test_setup));
  g_assert(ct_addTearDown(t, value_test_teardown));
  g_assert(ct_addTestFun(t, test_value_new));
  g_assert(ct_addTestFun(t, test_value_set_image_data));
  g_assert(ct_addTestFun(t, test_value_set_image_data_rect));
  g_assert(ct_addTestFun(t, test_value_set_image_data_tile));

  return t; 
}
