#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglSimpleImageMgr *simple_image_man; 

static void
test_fill_g_object_new(Test *test)
{
  {
    GeglFill * fill = g_object_new (GEGL_TYPE_FILL, NULL);  

    ct_test(test, fill != NULL);
    ct_test(test, GEGL_IS_FILL(fill));
    ct_test(test, g_type_parent(GEGL_TYPE_FILL) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglFill", g_type_name(GEGL_TYPE_FILL)));

    g_object_unref(fill);
  }

  {
    GeglFill * fill;
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

    fill = g_object_new (GEGL_TYPE_FILL, 
                         "fillcolor", color, 
                         NULL);  

    ct_test(test, fill != NULL);

    check_color = gegl_fill_get_fill_color(fill);
    check_chans = gegl_color_get_channel_values(check_color);

    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[0].f, chans[0].f));
    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[1].f, chans[1].f));
    ct_test(test, GEGL_FLOAT_EQUAL(check_chans[2].f, chans[2].f));

    g_object_unref(fill);
    g_object_unref(color);
    g_object_unref(rgb_float);
  }
}

static void
test_fill_apply(Test *test)
{
  {
    GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
    GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                      "colormodel", rgb_float,  
                                      NULL);  
    GeglChannelValue * chans = gegl_color_get_channel_values(color);
    GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                                   "colormodel", rgb_float,
                                                   "width", SAMPLED_IMAGE_WIDTH, 
                                                   "height", SAMPLED_IMAGE_HEIGHT,
                                                   NULL);  

    GeglOp *op = g_object_new(GEGL_TYPE_FILL, 
                              "fillcolor", color, 
                              NULL);

    chans[0].f = .1;
    chans[1].f = .2;
    chans[2].f = .3;

    gegl_op_apply(op, sampled_image, NULL); 
    ct_test(test, check_rgb_float_pixel(GEGL_IMAGE(sampled_image), .1, .2, .3));  

    g_object_unref(op);
    g_object_unref(sampled_image);
    g_object_unref(color);
    g_object_unref(rgb_float);
  }
}

static void
fill_test_setup(Test *test)
{
  simple_image_man = GEGL_SIMPLE_IMAGE_MGR(gegl_image_mgr_instance());
}

static void
fill_test_teardown(Test *test)
{
  g_object_unref (simple_image_man);
}

Test *
create_fill_test()
{
  Test* t = ct_create("GeglFillTest");

  g_assert(ct_addSetUp(t, fill_test_setup));
  g_assert(ct_addTearDown(t, fill_test_teardown));
  g_assert(ct_addTestFun(t, test_fill_g_object_new));
  g_assert(ct_addTestFun(t, test_fill_apply));

  return t; 
}
