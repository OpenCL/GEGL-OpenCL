#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_image_data_g_object_new(Test *test)
{
  {
    GeglImageData * image_data = g_object_new (GEGL_TYPE_IMAGE_DATA, NULL);  

    ct_test(test, image_data != NULL);
    ct_test(test, GEGL_IS_IMAGE_DATA(image_data));
    ct_test(test, g_type_parent(GEGL_TYPE_IMAGE_DATA) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglImageData", g_type_name(GEGL_TYPE_IMAGE_DATA)));

    g_object_unref(image_data);
  }
}

static void
test_image_data_construct_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImageData * image_data = g_object_new (GEGL_TYPE_IMAGE_DATA, 
                                               "color_model", pixel_rgb_float, 
                                               NULL);  

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_data_get_color_model(image_data));
    ct_test(test, NULL == gegl_image_data_get_tile(image_data));

    g_object_unref(image_data);
  }
}

static void
test_image_data_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImageData * image_data = g_object_new (GEGL_TYPE_IMAGE_DATA,  NULL);  

    gegl_image_data_set_color_model(image_data, pixel_rgb_float);

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_data_get_color_model(image_data));

    g_object_unref(image_data);
  }
}

static void
test_image_data_create_tile(Test *test)
{
  {
    GeglRect area = {0,0,2,2};
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");

    GeglImageData * image_data = g_object_new (GEGL_TYPE_IMAGE_DATA, NULL);  
    gegl_image_data_create_tile(image_data, pixel_rgb_float, &area);

    ct_test(test, NULL != gegl_image_data_get_tile(image_data));
    ct_test(test, pixel_rgb_float == gegl_image_data_get_color_model(image_data));

    g_object_unref(image_data);
  }
}

static void
image_data_test_setup(Test *test)
{
}

static void
image_data_test_teardown(Test *test)
{
}

Test *
create_image_data_test()
{
  Test* t = ct_create("GeglImageDataTest");

  g_assert(ct_addSetUp(t, image_data_test_setup));
  g_assert(ct_addTearDown(t, image_data_test_teardown));
  g_assert(ct_addTestFun(t, test_image_data_g_object_new));
  g_assert(ct_addTestFun(t, test_image_data_construct_properties));
  g_assert(ct_addTestFun(t, test_image_data_properties));
  g_assert(ct_addTestFun(t, test_image_data_create_tile));

  return t; 
}
