#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_color_space_g_object_new(Test *test)
{
  {
    GeglColorSpace * color_space = g_object_new (GEGL_TYPE_COLOR_SPACE_RGB, NULL);  

    ct_test(test, color_space != NULL);
    ct_test(test, GEGL_IS_COLOR_SPACE_RGB(color_space));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR_SPACE_RGB) == GEGL_TYPE_COLOR_SPACE);
    ct_test(test, !strcmp("GeglColorSpaceRgb", g_type_name(GEGL_TYPE_COLOR_SPACE_RGB)));

    g_object_unref(color_space);
  }
}

static void
test_color_space_rgb_settings(Test *test)
{
  {
    GeglColorSpace * color_space = g_object_new (GEGL_TYPE_COLOR_SPACE_RGB, NULL);  

    ct_test(test, GEGL_COLOR_SPACE_RGB == gegl_color_space_color_space_type(color_space)); 
    ct_test(test, 3 == gegl_color_space_num_components(color_space)); 
    ct_test(test, !strcmp("rgb", gegl_color_space_name(color_space)));
    ct_test(test, !strcmp("red", gegl_color_space_component_name(color_space, 0)));
    ct_test(test, !strcmp("green", gegl_color_space_component_name(color_space, 1)));
    ct_test(test, !strcmp("blue", gegl_color_space_component_name(color_space, 2)));

    g_object_unref(color_space);
  }
}

static void
test_color_space_gray_settings(Test *test)
{
  {
    GeglColorSpace * color_space = g_object_new (GEGL_TYPE_COLOR_SPACE_GRAY, NULL);  

    ct_test(test, GEGL_COLOR_SPACE_GRAY == gegl_color_space_color_space_type(color_space)); 
    ct_test(test, 1 == gegl_color_space_num_components(color_space)); 
    ct_test(test, !strcmp("gray", gegl_color_space_name(color_space)));
    ct_test(test, !strcmp("gray", gegl_color_space_component_name(color_space, 0)));

    g_object_unref(color_space);
  }
}

static void
color_space_test_setup(Test *test)
{
}

static void
color_space_test_teardown(Test *test)
{
}

Test *
create_color_space_test()
{
  Test* t = ct_create("GeglColorSpaceTest");

  g_assert(ct_addSetUp(t, color_space_test_setup));
  g_assert(ct_addTearDown(t, color_space_test_teardown));
  g_assert(ct_addTestFun(t, test_color_space_g_object_new));
  g_assert(ct_addTestFun(t, test_color_space_rgb_settings));
  g_assert(ct_addTestFun(t, test_color_space_gray_settings));

  return t; 
}
