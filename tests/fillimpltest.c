#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static void
test_fill_impl_g_object_new(Test *test)
{
  {
    GeglFillImpl * fill_impl = g_object_new (GEGL_TYPE_FILL_IMPL, NULL);  

    ct_test(test, fill_impl != NULL);
    ct_test(test, GEGL_IS_FILL_IMPL(fill_impl));
    ct_test(test, g_type_parent(GEGL_TYPE_FILL_IMPL) == GEGL_TYPE_POINT_OP_IMPL);
    ct_test(test, !strcmp("GeglFillImpl", g_type_name(GEGL_TYPE_FILL_IMPL)));

    g_object_unref(fill_impl);
  }

  {
    GeglFillImpl * fill_impl;
    GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "colormodel", rgb_float,  
                                      NULL);  
    GeglChannelValue * chans = gegl_color_get_channel_values(color);

    GeglColor * check_color = NULL;
    GeglChannelValue * check_chans = NULL;

    chans[0].f = .1;
    chans[1].f = .2;
    chans[2].f = .3;

    fill_impl = g_object_new (GEGL_TYPE_FILL_IMPL, NULL);  
    gegl_fill_impl_set_fill_color(fill_impl, color);

    ct_test(test, fill_impl != NULL);

    check_color = gegl_fill_impl_get_fill_color(fill_impl);
    check_chans = gegl_color_get_channel_values(check_color);

    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[0].f, chans[0].f));
    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[1].f, chans[1].f));
    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[2].f, chans[2].f));

    g_object_unref(fill_impl);
    g_object_unref(color);
    g_object_unref(rgb_float);
  }
}

static void
fill_impl_test_setup(Test *test)
{
}

static void
fill_impl_test_teardown(Test *test)
{
}

Test *
create_fill_impl_test()
{
  Test* t = ct_create("GeglFillImplTest");

  g_assert(ct_addSetUp(t, fill_impl_test_setup));
  g_assert(ct_addTearDown(t, fill_impl_test_teardown));
  g_assert(ct_addTestFun(t, test_fill_impl_g_object_new));

  return t; 
}
