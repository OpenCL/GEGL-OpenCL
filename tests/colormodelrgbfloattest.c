#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
do_rgba_float_tests (Test * test, GeglColorModel *color_model)
{
  ct_test(test, color_model != NULL);
  ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_model_color_space(color_model)); 
  ct_test(test, GEGL_COLOR_ALPHA_SPACE_RGBA == 
                gegl_color_model_color_alpha_space(color_model)); 
  ct_test(test, TRUE == gegl_color_model_has_alpha(color_model)); 
  ct_test(test, 4 == gegl_color_model_num_channels(color_model)); 
  ct_test(test, 3 == gegl_color_model_num_colors(color_model)); 
  ct_test(test, 3 == gegl_color_model_alpha_channel(color_model)); 

  ct_test(test, GEGL_FLOAT == gegl_color_model_data_type(color_model)); 
  ct_test(test, 4 == gegl_color_model_bytes_per_channel(color_model)); 
  ct_test(test, 16 == gegl_color_model_bytes_per_pixel(color_model)); 
  ct_test(test, GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT == 
                gegl_color_model_color_model_type(color_model)); 
}

static void
do_rgb_float_tests (Test * test, GeglColorModel *color_model)
{
  ct_test(test, color_model != NULL);
  ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_model_color_space(color_model)); 
  ct_test(test, GEGL_COLOR_ALPHA_SPACE_RGB == 
                gegl_color_model_color_alpha_space(color_model)); 
  ct_test(test, FALSE == gegl_color_model_has_alpha(color_model)); 
  ct_test(test, 3 == gegl_color_model_num_channels(color_model)); 
  ct_test(test, 3 == gegl_color_model_num_colors(color_model)); 
  ct_test(test, -1 == gegl_color_model_alpha_channel(color_model)); 

  ct_test(test, GEGL_FLOAT == gegl_color_model_data_type(color_model)); 
  ct_test(test, 4 == gegl_color_model_bytes_per_channel(color_model)); 
  ct_test(test, 12 == gegl_color_model_bytes_per_pixel(color_model)); 
  ct_test(test, GEGL_COLOR_MODEL_TYPE_RGB_FLOAT == 
                gegl_color_model_color_model_type(color_model)); 
}

static void
test_color_model_rgb_float_g_object_new(Test *test)
{
  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, NULL);  

    ct_test(test, color_model != NULL);
    ct_test(test, GEGL_IS_COLOR_MODEL_RGB_FLOAT(color_model));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR_MODEL_RGB_FLOAT) == GEGL_TYPE_COLOR_MODEL_RGB);
    ct_test(test, !strcmp("GeglColorModelRgbFloat", g_type_name(GEGL_TYPE_COLOR_MODEL_RGB_FLOAT)));

    g_object_unref(color_model);
  }

  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                                 "hasalpha", TRUE,
                                                 NULL);  
    do_rgba_float_tests(test, color_model); 
    g_object_unref(color_model);
  }

  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                                 "hasalpha", FALSE,
                                                 NULL);  

    do_rgb_float_tests(test, color_model); 
    g_object_unref(color_model);
  }
}

static void
test_color_model_rgb_float_convert_interfaces(Test *test)
{
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, NULL);  

    ct_test(test,!strcmp("FromRgbFloat", gegl_color_model_get_convert_interface_name(color_model)));
    ct_test(test, NULL != gegl_object_query_interface(GEGL_OBJECT(color_model),"FromRgbFloat"));
    ct_test(test, NULL != gegl_object_query_interface(GEGL_OBJECT(color_model),"FromRgbU8"));
    ct_test(test, NULL != gegl_object_query_interface(GEGL_OBJECT(color_model),"FromGrayFloat"));

    g_object_unref(color_model);
}

static void
color_model_rgb_float_test_setup(Test *test)
{
}

static void
color_model_rgb_float_test_teardown(Test *test)
{
}

Test *
create_color_model_rgb_float_test()
{
  Test* t = ct_create("GeglColorModelRgbFloatTest");

  g_assert(ct_addSetUp(t, color_model_rgb_float_test_setup));
  g_assert(ct_addTearDown(t, color_model_rgb_float_test_teardown));
  g_assert(ct_addTestFun(t, test_color_model_rgb_float_g_object_new));
  g_assert(ct_addTestFun(t, test_color_model_rgb_float_convert_interfaces));

  return t; 
}
