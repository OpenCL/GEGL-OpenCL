#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_image_buffer_g_object_new(Test *test)
{
  {
    GeglImageBuffer * image_buffer = g_object_new (GEGL_TYPE_IMAGE_BUFFER, NULL);  

    ct_test(test, image_buffer != NULL);
    ct_test(test, GEGL_IS_IMAGE_BUFFER(image_buffer));
    ct_test(test, g_type_parent(GEGL_TYPE_IMAGE_BUFFER) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglImageBuffer", g_type_name(GEGL_TYPE_IMAGE_BUFFER)));

    g_object_unref(image_buffer);
  }
}

static void
test_image_buffer_construct_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImageBuffer * image_buffer = g_object_new (GEGL_TYPE_IMAGE_BUFFER, 
                                               "color_model", pixel_rgb_float, 
                                               NULL);  

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_buffer_get_color_model(image_buffer));
    ct_test(test, NULL == gegl_image_buffer_get_tile(image_buffer));

    g_object_unref(image_buffer);
  }
}

static void
test_image_buffer_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImageBuffer * image_buffer = g_object_new (GEGL_TYPE_IMAGE_BUFFER,  NULL);  

    gegl_image_buffer_set_color_model(image_buffer, pixel_rgb_float);

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_buffer_get_color_model(image_buffer));

    g_object_unref(image_buffer);
  }
}

static void
test_image_buffer_create_tile(Test *test)
{
  {
    GeglRect area = {0,0,2,2};
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");

    GeglImageBuffer * image_buffer = g_object_new (GEGL_TYPE_IMAGE_BUFFER, NULL);  
    gegl_image_buffer_create_tile(image_buffer, pixel_rgb_float, &area);

    ct_test(test, NULL != gegl_image_buffer_get_tile(image_buffer));
    ct_test(test, pixel_rgb_float == gegl_image_buffer_get_color_model(image_buffer));

    g_object_unref(image_buffer);
  }
}

static void
image_buffer_test_setup(Test *test)
{
}

static void
image_buffer_test_teardown(Test *test)
{
}

Test *
create_image_buffer_test()
{
  Test* t = ct_create("GeglImageBufferTest");

  g_assert(ct_addSetUp(t, image_buffer_test_setup));
  g_assert(ct_addTearDown(t, image_buffer_test_teardown));
  g_assert(ct_addTestFun(t, test_image_buffer_g_object_new));
  g_assert(ct_addTestFun(t, test_image_buffer_construct_properties));
  g_assert(ct_addTestFun(t, test_image_buffer_properties));
  g_assert(ct_addTestFun(t, test_image_buffer_create_tile));

  return t; 
}
