#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_color_g_object_new(Test *test)
{
  {
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, NULL);  

    ct_test(test, color != NULL);
    ct_test(test, GEGL_IS_COLOR(color));
    ct_test(test, g_type_parent(GEGL_TYPE_COLOR) == GEGL_TYPE_OBJECT);
    ct_test(test, !strcmp("GeglColor", g_type_name(GEGL_TYPE_COLOR)));

    g_object_unref(color);
  }
}

static void
test_color_properties(Test *test)
{
  {
    gfloat r, g, b;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgb-float", .1, .2, .3, 
                                      NULL);  

    g_object_get(color, "rgb-float", &r, &g, &b, NULL);

    ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, b));

    g_object_unref(color);
  }

  {
    gfloat r, g, b, a;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgba-float", .1, .2, .3, .4,
                                      NULL);  

    g_object_get(color, "rgba-float", &r, &g, &b, &a, NULL);

    ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, b));
    ct_test(test, GEGL_FLOAT_EQUAL(.4, a));

    g_object_unref(color);
  }
}

static void
test_color_properties_convert(Test *test)
{
  {
    gfloat r, g, b, a;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgb-float", .1, .2, .3,
                                      NULL);  

    g_object_get(color, "rgba-float", &r, &g, &b, &a, NULL);

    ct_test(test, GEGL_FLOAT_EQUAL(.1, r));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, g));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, b));
    ct_test(test, GEGL_FLOAT_EQUAL(1.0, a));

    g_object_unref(color);
  }
}

static void
test_color_read_only_properties(Test *test)
{
  {
    guint8 r, g, b;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgb-float", 1.5, .2, .3,
                                      NULL);  

    /* Read 8 bit version of the data */
    g_object_get(color, "rgb-uint8", &r, &g, &b, NULL);

    ct_test(test, r == 255);  /* value is clamped at 255 */
    ct_test(test, g == ROUND(.2*255.0));
    ct_test(test, b == ROUND(.3*255.0));

    g_object_unref(color);
  }
}

static void
test_color_read_components_property(Test *test)
{
  {
    gint num_components;
    gfloat *components;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgba-float", 1.5, .2, .3, .4,
                                      NULL);  

    /* Read components */
    g_object_get(color, "components", &num_components, &components, NULL);

    ct_test(test, 4 == num_components); 
    ct_test(test, GEGL_FLOAT_EQUAL(1.5, components[0]));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, components[1]));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, components[2]));
    ct_test(test, GEGL_FLOAT_EQUAL(.4, components[3]));

    g_object_unref(color);
    g_free(components);
  }
}

static void
test_color_read_color_components_property(Test *test)
{
  {
    gint num_color_components;
    gfloat *color_components;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgba-float", 1.5, .2, .3, .4,
                                      NULL);  

    /* Read color components */
    g_object_get(color, 
                 "color-components", &num_color_components, &color_components, 
                 NULL);

    ct_test(test, 3 == num_color_components); 
    ct_test(test, GEGL_FLOAT_EQUAL(1.5, color_components[0]));
    ct_test(test, GEGL_FLOAT_EQUAL(.2, color_components[1]));
    ct_test(test, GEGL_FLOAT_EQUAL(.3, color_components[2]));

    g_object_unref(color);
    g_free(color_components);
  }
}

static void
test_color_read_color_space_property(Test *test)
{
  {
    gchar *color_space;
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "rgba-float", 1.5, .2, .3, .4,
                                      NULL);  

    /* Read color space */
    g_object_get(color, 
                 "color-space", &color_space, 
                 NULL);

    ct_test(test, !strcmp("rgb", color_space));

    g_object_unref(color);
    g_free(color_space);
  }
}

static void
color_test_setup(Test *test)
{
}

static void
color_test_teardown(Test *test)
{
}

Test *
create_color_test()
{
  Test* t = ct_create("GeglColorTest");

  g_assert(ct_addSetUp(t, color_test_setup));
  g_assert(ct_addTearDown(t, color_test_teardown));
  g_assert(ct_addTestFun(t, test_color_g_object_new));
  g_assert(ct_addTestFun(t, test_color_properties));
  g_assert(ct_addTestFun(t, test_color_read_only_properties));
  g_assert(ct_addTestFun(t, test_color_read_components_property));
  g_assert(ct_addTestFun(t, test_color_read_color_components_property));
  g_assert(ct_addTestFun(t, test_color_read_color_space_property));

  return t; 
}
