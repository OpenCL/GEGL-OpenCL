#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_color_model_g_object_new(Test *test)
{
  {
    GeglColorModel *color_model = gegl_color_model_instance("RgbFloat");  

    ct_test(test, color_model != NULL);
    ct_test(test, GEGL_IS_COLOR_MODEL(color_model));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR_MODEL) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglColorModel", g_type_name(GEGL_TYPE_COLOR_MODEL)));

    g_object_unref(color_model);
  }

  {
    GeglColorModel *color_model = gegl_color_model_instance("RgbFloat"); 
    ct_test(test, color_model != NULL);
    ct_test(test, FALSE == gegl_color_model_has_alpha(color_model));

    g_object_unref(color_model);
  }

  {
    GeglColorModel *color_model = gegl_color_model_instance("RgbFloatAlpha"); 
    ct_test(test, color_model != NULL);
    ct_test(test, TRUE == gegl_color_model_has_alpha(color_model));

    g_object_unref(color_model);
  }
}

static void
test_color_model_g_object_get(Test *test)
{
  {
    gboolean has_alpha;
    GeglColorModel *color_model = gegl_color_model_instance("RgbFloat"); 

    g_object_get(color_model, "hasalpha", &has_alpha, NULL);

    ct_test(test, FALSE == gegl_color_model_has_alpha(color_model));

    g_object_unref(color_model);
  }

  {
    gboolean has_alpha;
    GeglColorModel *color_model = gegl_color_model_instance("RgbFloatAlpha"); 

    g_object_get(color_model, "hasalpha", &has_alpha, NULL);

    ct_test(test, TRUE == gegl_color_model_has_alpha(color_model));

    g_object_unref(color_model);
  }
}

static void
color_model_test_setup(Test *test)
{
}

static void
color_model_test_teardown(Test *test)
{
}

Test *
create_color_model_test()
{
  Test* t = ct_create("GeglColorModelTest");

  g_assert(ct_addSetUp(t, color_model_test_setup));
  g_assert(ct_addTearDown(t, color_model_test_teardown));
  g_assert(ct_addTestFun(t, test_color_model_g_object_new));
  g_assert(ct_addTestFun(t, test_color_model_g_object_get));

  return t; 
}
