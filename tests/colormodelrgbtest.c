#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>


static void
test_color_model_rgb_g_object_new(Test *test)
{
  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, NULL);  

    ct_test(test, color_model != NULL);
    ct_test(test, GEGL_IS_COLOR_MODEL_RGB(color_model));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR_MODEL_RGB) == GEGL_TYPE_COLOR_MODEL);
    ct_test(test, !strcmp("GeglColorModelRgb", g_type_name(GEGL_TYPE_COLOR_MODEL_RGB)));

    g_object_unref(color_model);
  }

  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                                 "hasalpha", TRUE,
                                                 NULL);  


    ct_test(test, color_model != NULL);

    ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_model_color_space(color_model)); 
    ct_test(test, GEGL_COLOR_ALPHA_SPACE_RGBA == gegl_color_model_color_alpha_space(color_model)); 
    ct_test(test, TRUE == gegl_color_model_has_alpha(color_model)); 
    ct_test(test, 4 == gegl_color_model_num_channels(color_model)); 
    ct_test(test, 3 == gegl_color_model_num_colors(color_model)); 
    ct_test(test, 3 == gegl_color_model_alpha_channel(color_model)); 

    g_object_unref(color_model);
  }

  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                                 "hasalpha", FALSE,
                                                 NULL);  

    ct_test(test, color_model != NULL);

    ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_model_color_space(color_model)); 
    ct_test(test, GEGL_COLOR_ALPHA_SPACE_RGB == gegl_color_model_color_alpha_space(color_model)); 
    ct_test(test, FALSE == gegl_color_model_has_alpha(color_model)); 
    ct_test(test, 3 == gegl_color_model_num_channels(color_model)); 
    ct_test(test, 3 == gegl_color_model_num_colors(color_model)); 
    ct_test(test, -1 == gegl_color_model_alpha_channel(color_model)); 

    g_object_unref(color_model);
  }

  {
    GeglColorModel * color_model = g_object_new (GEGL_TYPE_COLOR_MODEL_RGB_FLOAT, 
                                                 NULL);  

    ct_test(test, color_model != NULL);

    ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_model_color_space(color_model)); 
    ct_test(test, GEGL_COLOR_ALPHA_SPACE_RGB == gegl_color_model_color_alpha_space(color_model)); 
    ct_test(test, FALSE == gegl_color_model_has_alpha(color_model)); 
    ct_test(test, 3 == gegl_color_model_num_channels(color_model)); 
    ct_test(test, 3 == gegl_color_model_num_colors(color_model)); 
    ct_test(test, -1 == gegl_color_model_alpha_channel(color_model)); 

    g_object_unref(color_model);
  }
}

static void
color_model_rgb_test_setup(Test *test)
{
}

static void
color_model_rgb_test_teardown(Test *test)
{
}

Test *
create_color_model_rgb_test()
{
  Test* t = ct_create("GeglColorModelRgbTest");

  g_assert(ct_addSetUp(t, color_model_rgb_test_setup));
  g_assert(ct_addTearDown(t, color_model_rgb_test_teardown));
  g_assert(ct_addTestFun(t, test_color_model_rgb_g_object_new));

  return t; 
}
