#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_image_g_object_new(Test *test)
{
  {
    GeglImage * image = g_object_new (GEGL_TYPE_IMAGE, NULL);  

    ct_test(test, image != NULL);
    ct_test(test, GEGL_IS_IMAGE(image));
    ct_test(test, g_type_parent(GEGL_TYPE_IMAGE) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglImage", g_type_name(GEGL_TYPE_IMAGE)));

    g_object_unref(image);
  }
}

static void
test_image_construct_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImage * image = g_object_new (GEGL_TYPE_IMAGE, 
                                      "color_model", pixel_rgb_float, 
                                      NULL);  

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_get_color_model(image));
    ct_test(test, NULL == gegl_image_get_tile(image));

    g_object_unref(image);
  }
}

static void
test_image_properties(Test *test)
{
  {
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");
    GeglImage * image = g_object_new (GEGL_TYPE_IMAGE,  NULL);  

    gegl_image_set_color_model(image, pixel_rgb_float);

    ct_test(test, pixel_rgb_float == (GeglColorModel*)gegl_image_get_color_model(image));

    g_object_unref(image);
  }
}

static void
test_image_create_tile(Test *test)
{
  {
    GeglRect area = {0,0,2,2};
    GeglColorModel * pixel_rgb_float = gegl_color_model_instance("rgb-float");

    GeglImage * image = g_object_new (GEGL_TYPE_IMAGE, NULL);  
    gegl_image_create_tile(image, pixel_rgb_float, &area);

    ct_test(test, NULL != gegl_image_get_tile(image));
    ct_test(test, pixel_rgb_float == gegl_image_get_color_model(image));

    g_object_unref(image);
  }
}

static void
image_test_setup(Test *test)
{
}

static void
image_test_teardown(Test *test)
{
}

Test *
create_image_test()
{
  Test* t = ct_create("GeglImageTest");

  g_assert(ct_addSetUp(t, image_test_setup));
  g_assert(ct_addTearDown(t, image_test_teardown));
  g_assert(ct_addTestFun(t, test_image_g_object_new));
  g_assert(ct_addTestFun(t, test_image_construct_properties));
  g_assert(ct_addTestFun(t, test_image_properties));
  g_assert(ct_addTestFun(t, test_image_create_tile));

  return t; 
}
