#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglImageBuffer * image_buffer;

#define TILE_WIDTH 2 
#define TILE_HEIGHT 2 
#define TILE_X 0 
#define TILE_Y 0 
static GeglRect tile_rect = {TILE_X, TILE_Y, TILE_WIDTH, TILE_HEIGHT};

#define ITER_AREA_WIDTH 1 
#define ITER_AREA_HEIGHT 1 
#define ITER_AREA_X 1 
#define ITER_AREA_Y 1 
static GeglRect iter_area = {ITER_AREA_X,ITER_AREA_Y,ITER_AREA_WIDTH,ITER_AREA_HEIGHT};

static void
test_image_buffer_iterator_g_object_new(Test *test)
{
  {
    GeglImageBufferIterator * iterator = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                                                     "image_buffer", image_buffer,
                                                     "area", &iter_area,
                                                     NULL);  

    ct_test(test, iterator != NULL);
    ct_test(test, GEGL_IS_IMAGE_BUFFER_ITERATOR(iterator));
    ct_test(test, g_type_parent(GEGL_TYPE_IMAGE_BUFFER_ITERATOR) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglImageBufferIterator", g_type_name(GEGL_TYPE_IMAGE_BUFFER_ITERATOR)));

    g_object_unref(iterator);
  }
}

static void
test_image_buffer_iterator_properties(Test *test)
{
  {
    GeglRect area;
    gpointer * color_chans;
    GeglColorModel *rgba_float = gegl_color_model_instance("rgba-float");
    GeglImageBufferIterator * iterator = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                                                     "image_buffer", image_buffer,
                                                     "area", &iter_area,
                                                     NULL);  

    ct_test(test, image_buffer == gegl_image_buffer_iterator_get_image_buffer(iterator));
    ct_test(test, rgba_float == gegl_image_buffer_iterator_get_color_model(iterator));
    ct_test(test, 3 == gegl_image_buffer_iterator_get_num_colors(iterator));
    ct_test(test, NULL != gegl_image_buffer_iterator_alpha_channel(iterator));
    color_chans = gegl_image_buffer_iterator_color_channels(iterator);
    ct_test(test, NULL != color_chans);
    g_free(color_chans);

    gegl_image_buffer_iterator_get_rect(iterator, &area);

    ct_test(test, area.x == ITER_AREA_X);
    ct_test(test, area.y == ITER_AREA_Y);
    ct_test(test, area.w == ITER_AREA_WIDTH);
    ct_test(test, area.h == ITER_AREA_HEIGHT);

    g_object_unref(iterator);
  }
}

static void
test_image_buffer_iterator_color_alpha_channels(Test *test)
{
  {
    GeglImageBufferIterator * iterator = g_object_new (GEGL_TYPE_IMAGE_BUFFER_ITERATOR, 
                                                     "image_buffer", image_buffer,
                                                     "area", &iter_area,
                                                     NULL);  

    GeglTile * tile = gegl_image_buffer_get_tile(image_buffer);
    GeglDataBuffer *data_buffer = gegl_tile_get_data_buffer(tile);
    gfloat ** banks = (gfloat**)gegl_data_buffer_banks_data(data_buffer);
    gfloat ** color_chans = (gfloat**)gegl_image_buffer_iterator_color_channels(iterator);
    gfloat * alpha_chan = (gfloat*)gegl_image_buffer_iterator_alpha_channel(iterator);

    /* 
       In the data buffer we have (as floats):

       banks[0] = x x 
                  x x          
       banks[1] = x x 
                  x x 
       banks[2] = x x 
                  x x 
       banks[3] = x x 
                  x x 

       tiles rect is {0,0,2,2}
       iters rect is {1,1,1,1}

       so that the color_chans array should point to the samples in (1,1) spot.
       That is the 4th sample in each bank. 

     */

    ct_test(test, color_chans[0] == banks[0] + 3); 
    ct_test(test, color_chans[1] == banks[1] + 3); 
    ct_test(test, color_chans[2] == banks[2] + 3); 
    ct_test(test, alpha_chan     == banks[3] + 3); 

    g_free(color_chans);
    g_object_unref(iterator);
  }
}

static void
image_buffer_iterator_test_setup(Test *test)
{
  image_buffer = g_object_new (GEGL_TYPE_IMAGE_BUFFER, NULL);  
  gegl_image_buffer_create_tile(image_buffer, 
                                gegl_color_model_instance("rgba-float"), 
                                &tile_rect);
}

static void
image_buffer_iterator_test_teardown(Test *test)
{
  g_object_unref(image_buffer);
}

Test *
create_image_buffer_iterator_test()
{
  Test* t = ct_create("GeglImageBufferIteratorTest");

  g_assert(ct_addSetUp(t, image_buffer_iterator_test_setup));
  g_assert(ct_addTearDown(t, image_buffer_iterator_test_teardown));
  g_assert(ct_addTestFun(t, test_image_buffer_iterator_g_object_new));
  g_assert(ct_addTestFun(t, test_image_buffer_iterator_properties));
  g_assert(ct_addTestFun(t, test_image_buffer_iterator_color_alpha_channels));

  return t; 
}
