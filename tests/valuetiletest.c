#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define AREA_WIDTH 10 
#define AREA_HEIGHT 10 

static void
test_value_tile_set(Test *test)
{
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglColorModel *color_model = gegl_color_model_instance("RgbFloat");
  GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  
  GValue *value =  g_new0(GValue, 1); 

  g_value_init(value, GEGL_TYPE_TILE);
  g_value_set_tile(value, tile);
  ct_test(test, tile == g_value_get_tile(value));
  g_value_unset(value);

  g_free(value);

  g_object_unref(tile);
}

static void
test_value_tile_copy(Test *test)
{
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglColorModel *color_model = gegl_color_model_instance("RgbFloat");
  GeglTile * tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  

  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 

  g_value_init(dest_value, GEGL_TYPE_TILE);
  g_value_init(src_value, GEGL_TYPE_TILE);

  g_value_set_tile(src_value, tile);

  g_value_copy(src_value, dest_value);

  ct_test(test, tile == g_value_get_tile(dest_value));

  g_value_unset(dest_value);
  g_value_unset(src_value);

  g_free(dest_value);
  g_free(src_value);

  g_object_unref(tile);
}

static void
test_value_tile_compatible(Test *test)
{
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglColorModel *color_model = gegl_color_model_instance("RgbFloat");
  GeglTile * src_tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  
  GeglTile * dest_tile = g_object_new (GEGL_TYPE_TILE, 
                                  "area", &area, 
                                  "colormodel", color_model,
                                   NULL);  

  GValue * src_value =  g_new0(GValue, 1); 
  GValue * dest_value =  g_new0(GValue, 1); 
  GValue * float_value = g_new0(GValue, 1);
  GValue * int_value = g_new0(GValue, 1);

  g_value_init(dest_value, GEGL_TYPE_TILE);
  g_value_init(src_value, GEGL_TYPE_TILE);
  g_value_init(float_value, G_TYPE_FLOAT);
  g_value_init(int_value, G_TYPE_INT);

  g_value_set_tile(src_value, src_tile);
  g_value_set_float(float_value, 3.4);

  ct_test(test, g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));
  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(dest_value)));
  ct_test(test, g_value_transform(src_value, dest_value));
  ct_test(test, !g_value_type_compatible(G_VALUE_TYPE(src_value), G_VALUE_TYPE(float_value)));
  ct_test(test, !g_value_type_transformable(G_VALUE_TYPE(src_value), G_VALUE_TYPE(float_value)));
  ct_test(test, !g_value_transform(src_value, float_value));

  ct_test(test, g_value_type_transformable(G_VALUE_TYPE(float_value), G_VALUE_TYPE(int_value)));
  ct_test(test, g_value_transform(float_value, int_value));

  /* float to int conversion just truncates */
  ct_test(test, 3 == g_value_get_int(int_value));

  g_value_unset(dest_value);
  g_value_unset(src_value);
  g_value_unset(float_value);

  g_free(dest_value);
  g_free(src_value);
  g_free(float_value);

  g_object_unref(src_tile);
  g_object_unref(dest_tile);
}

static void
test_value_tile_param_spec_validate(Test *test)
{
  GeglTile *converted_tile;
  gfloat converted_val;
  GeglRect area = {0,0,AREA_WIDTH,AREA_HEIGHT};
  GeglRect smaller_area = {0,0,AREA_WIDTH-1,AREA_HEIGHT};
  GeglRect bigger_area = {0,0,AREA_WIDTH+1,AREA_HEIGHT};

  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
  GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");

  GeglOp *filled = testutils_rgb_float_sampled_image(AREA_WIDTH, 
                                                     AREA_HEIGHT, 
                                                     .1, .2, .3);
  GeglTile *tile = gegl_image_get_tile(GEGL_IMAGE(filled));

  GParamSpec *pspec0 = gegl_param_spec_tile("data0",
                                            "Data0",
                                            "data0",
                                            &smaller_area,
                                            rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec1 = gegl_param_spec_tile("data1",
                                            "Data1",
                                            "data1",
                                            &bigger_area,
                                            rgb_float,
                                            G_PARAM_READWRITE);

  GParamSpec *pspec2 = gegl_param_spec_tile("data2",
                                            "Data2",
                                            "data2",
                                            &area,
                                            gray_float,
                                            G_PARAM_READWRITE);

  GValue * value =  g_new0(GValue, 1); 
  g_value_init(value, GEGL_TYPE_TILE);

  /* tile rect contains spec rect */
  g_value_set_tile(value, tile);
  ct_test(test, !g_param_value_validate(pspec0, value)); /*returns true if modified*/

  /* tile rect does not contain spec rect, cant validate */
  g_value_set_tile(value, tile);
  ct_test(test, g_param_value_validate(pspec1, value));
  ct_test(test, NULL == g_value_get_tile(value));

  /* color models are different, can validate after converting */
  g_value_set_tile(value, tile);
  ct_test(test, g_param_value_validate(pspec2, value));

  /* This was an gray to rgb conversion */
  converted_val = .3*.1 + .59*.2 + .11*.3;

  converted_tile = g_value_get_tile(value);
  ct_test(test, NULL != converted_tile);

  ct_test(test, testutils_check_pixel_tile(converted_tile,  "GrayFloat", 
                                           converted_val, 0, 0, 0));

  g_value_unset(value);
  g_free(value);

  g_object_unref(filled);

  g_param_spec_unref(pspec0);
  g_param_spec_unref(pspec1);
  g_param_spec_unref(pspec2);
}

static void
value_tile_test_setup(Test *test)
{
}

static void
value_tile_test_teardown(Test *test)
{
}

Test *
create_value_tile_test()
{
  Test* t = ct_create("GeglValueTileTest");

  g_assert(ct_addSetUp(t, value_tile_test_setup));
  g_assert(ct_addTearDown(t, value_tile_test_teardown));
  g_assert(ct_addTestFun(t, test_value_tile_set));
  g_assert(ct_addTestFun(t, test_value_tile_copy));
  g_assert(ct_addTestFun(t, test_value_tile_compatible));
  g_assert(ct_addTestFun(t, test_value_tile_param_spec_validate));

  return t; 
}
