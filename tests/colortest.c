#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglColorModel * color_model;

static void
test_color_g_object_new(Test *test)
{
  {
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "colormodel", color_model,  
                                      NULL);  

    ct_test(test, color != NULL);
    ct_test(test, GEGL_IS_COLOR(color));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglColor", g_type_name(GEGL_TYPE_COLOR)));

    g_object_unref(color);
  }
}

static void
test_color_get_num_channels(Test *test)
{
  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                    "colormodel", color_model,  
                                    NULL);  
  ct_test(test, 3 == gegl_color_get_num_channels(color));
  g_object_unref(color);
}

static void
color_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
}

static void
color_test_teardown(Test *test)
{
  g_object_unref(color_model);
}

Test *
create_color_test()
{
  Test* t = ct_create("GeglColorTest");

  g_assert(ct_addSetUp(t, color_test_setup));
  g_assert(ct_addTearDown(t, color_test_teardown));
  g_assert(ct_addTestFun(t, test_color_g_object_new));
  g_assert(ct_addTestFun(t, test_color_get_num_channels));

  return t; 
}
