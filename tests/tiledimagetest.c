#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "gegl-mock-filter.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define TILE_WIDTH 5 
#define TILE_HEIGHT 4 
static GeglRect tile_rect = {0,0,TILE_WIDTH,TILE_HEIGHT};
static GeglTile * tile;
static GeglColorModel * color_model;

static void
test_tiled_image_g_object_new(Test *test)
{
  {
    GeglTiledImage * tiled_image = g_object_new (GEGL_TYPE_TILED_IMAGE, 
                                                 "tile", tile,
                                                 NULL);  
    ct_test(test, tiled_image != NULL);
    ct_test(test, GEGL_IS_TILED_IMAGE(tiled_image));
    ct_test(test, g_type_parent(GEGL_TYPE_TILED_IMAGE) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglTiledImage", g_type_name(GEGL_TYPE_TILED_IMAGE)));

    g_object_unref(tiled_image);
  }
}

static void
test_tiled_image_apply(Test *test)
{
  GeglRect roi = {0,0,TILE_WIDTH,TILE_HEIGHT};

  GeglTiledImage * image0 = g_object_new (GEGL_TYPE_TILED_IMAGE, 
                                          "tile", tile,
                                          NULL);  

  GeglTiledImage * image1 = g_object_new (GEGL_TYPE_TILED_IMAGE, 
                                          "tile", tile,
                                          NULL);  

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                              "num_inputs", 2, 
                              "num_outputs", 1, 
                              "source0", image0,
                              "source1", image1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(image0);
  g_object_unref(image1);
}

static void
test_tiled_image_copy(Test *test)
{
  GeglOp *filled = testutils_rgb_float_sampled_image(TILE_WIDTH, 
                                                     TILE_HEIGHT, 
                                                     .1, .2, .3);
  GeglTile *tile = gegl_image_get_tile(GEGL_IMAGE(filled));
  GeglTiledImage * source = g_object_new (GEGL_TYPE_TILED_IMAGE, 
                                          "tile", tile,
                                          NULL);  
  GeglOp * copy = g_object_new (GEGL_TYPE_COPY, 
                                "source", source,
                                NULL);  
  gegl_op_apply(copy);
  ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(copy), .1,.2,.3));

  g_object_unref(copy);
  g_object_unref(source);
  g_object_unref(filled);
}

static void
tiled_image_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
  tile = g_object_new (GEGL_TYPE_TILE, 
                       "area", &tile_rect, 
                       "colormodel", color_model,
                       NULL);  

}

static void
tiled_image_test_teardown(Test *test)
{
  g_object_unref(tile);
}

Test *
create_tiled_image_test()
{
  Test* t = ct_create("GeglTiledImageTest");

  g_assert(ct_addSetUp(t, tiled_image_test_setup));
  g_assert(ct_addTearDown(t, tiled_image_test_teardown));
  g_assert(ct_addTestFun(t, test_tiled_image_g_object_new));
  g_assert(ct_addTestFun(t, test_tiled_image_apply));
  g_assert(ct_addTestFun(t, test_tiled_image_copy));

  return t; 
}
