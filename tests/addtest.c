#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglSampledImage *source0, *source1;
static GeglSampledImage *dest;

static GeglSimpleImageMgr *simple_image_man; 

static void
test_add_g_object_new(Test *test)
{
  {
    GeglOp * add = g_object_new (GEGL_TYPE_ADD, NULL);  

    ct_test(test, add != NULL);
    ct_test(test, GEGL_IS_ADD(add));
    ct_test(test, g_type_parent(GEGL_TYPE_ADD) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglAdd", g_type_name(GEGL_TYPE_ADD)));

    g_object_unref(add);
  }

  {
    GeglOp * add =  g_object_new (GEGL_TYPE_ADD, NULL);  

    ct_test(test, add != NULL);
    ct_test(test, NULL == gegl_op_get_source0(add));
    ct_test(test, NULL == gegl_op_get_source1(add));

    g_object_unref(add);
  }

  {
    GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                                 "source0", source0,
                                 "source1", source1,
                                 NULL);  

    ct_test(test, add != NULL);
    ct_test(test, (GeglOp*)source0 == gegl_op_get_source0(add));
    ct_test(test, (GeglOp*)source1 == gegl_op_get_source1(add));

    g_object_unref(add);
  }
}

static void
test_add_apply(Test *test)
{

  /* 
     add = source0 + source1 
     (.5,.7,.9) = (.1,.2,.3) + (.4,.5,.6)
  */

  GeglOp * add = g_object_new (GEGL_TYPE_ADD, 
                              "source0", source0,
                              "source1", source1,
                              NULL);  

  gegl_op_apply(add, dest, NULL); 

  ct_test(test, check_rgb_float_pixel(GEGL_IMAGE(dest), .1 + .4, .2 + .5, .3 + .6));  

  g_object_unref(add);
}

static void
add_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");


  source0 = make_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                        SAMPLED_IMAGE_HEIGHT, 
                                        .1, .2, .3);

  source1 = make_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                        SAMPLED_IMAGE_HEIGHT, 
                                        .4, .5, .6);

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  


  g_object_unref(rgb_float);

  simple_image_man = GEGL_SIMPLE_IMAGE_MGR(gegl_image_mgr_instance());
}

static void
add_test_teardown(Test *test)
{
  g_object_unref(source0);
  g_object_unref(source1);
  g_object_unref(dest);
  g_object_unref(simple_image_man);
}

Test *
create_add_test()
{
  Test* t = ct_create("GeglAddTest");

  g_assert(ct_addSetUp(t, add_test_setup));
  g_assert(ct_addTearDown(t, add_test_teardown));
  g_assert(ct_addTestFun(t, test_add_g_object_new));
  g_assert(ct_addTestFun(t, test_add_apply));

  return t; 
}
