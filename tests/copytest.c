#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglSampledImage * source;
static GeglSampledImage * gray_dest;
static GeglSampledImage * rgb_dest;

static GeglSimpleImageMgr *simple_image_man; 

static void
test_copy_g_object_new(Test *t)
{
  {
    GeglOp * copy = g_object_new (GEGL_TYPE_COPY, NULL);  

    ct_test(t, copy != NULL);
    ct_test(t, GEGL_IS_COPY(copy));
    ct_test(t, g_type_parent(GEGL_TYPE_COPY) == GEGL_TYPE_POINT_OP);
    ct_test(t, !strcmp("GeglCopy", g_type_name(GEGL_TYPE_COPY)));

    g_object_unref(copy);
  }

  {
    GeglCopy * copy = g_object_new (GEGL_TYPE_COPY, 
                                    "source0", source,
                                    NULL);  

    ct_test(t, copy != NULL);
    ct_test(t, 1 == gegl_node_num_inputs(GEGL_NODE(copy)));
    ct_test(t, (GeglOp*)source == gegl_op_get_source0(GEGL_OP(copy)));

    g_object_unref(copy);
  }
}

static void
test_copy_rgb_to_rgb_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                         "source0", source,
                                         NULL);

    gegl_op_apply(copy, rgb_dest, NULL); 

    ct_test(t, check_rgb_float_pixel(GEGL_IMAGE(rgb_dest), 
                                      .1 ,.2 ,.3));  

    g_object_unref(copy);
  }
}

static void
test_copy_rgb_to_null_dest_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "source0", source,
                                NULL);

    gegl_op_apply(copy, NULL, NULL); 

    ct_test(t, check_rgb_float_pixel(GEGL_IMAGE(copy), 
                                     .1 ,.2 ,.3));  

    g_object_unref(copy);
  }
}

static void
test_copy_rgb_to_gray_null_dest_apply(Test *t)
{
  {
    GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "colormodel", gray_float,
                                "source0", source,
                                NULL);

    gegl_op_apply(copy, NULL, NULL); 

    ct_test(t, check_gray_float_pixel(GEGL_IMAGE(copy), 
                                     .3*.1 + .59*.2 + .11*.3));  

    g_object_unref(copy);
    g_object_unref(gray_float);
  }
}


static void
test_copy_rgb_to_gray_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "source0", source,
                                NULL);

    gegl_op_apply(copy, gray_dest, NULL); 

    /* .3*r + .59*g + .11*b */
    ct_test(t, check_gray_float_pixel(GEGL_IMAGE(gray_dest), 
                                      .3*.1 + .59*.2 + .11*.3));  

    g_object_unref(copy);
  }
}

static void
test_copy_setup(Test *t)
{
  /* source is a rgb float image */
  source = make_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                       SAMPLED_IMAGE_HEIGHT, 
                                       .1, .2, .3);

  {
    /* dest is a gray float image */
    GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");
    gray_dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                              "colormodel", gray_float,
                              "width", SAMPLED_IMAGE_WIDTH, 
                              "height", SAMPLED_IMAGE_HEIGHT,
                              NULL);  
    g_object_unref(gray_float);
  }

  {
    /* dest is a gray float image */
    GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
    rgb_dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                             "colormodel", rgb_float,
                             "width", SAMPLED_IMAGE_WIDTH, 
                             "height", SAMPLED_IMAGE_HEIGHT,
                             NULL);  
    g_object_unref(rgb_float);
  }

  simple_image_man = GEGL_SIMPLE_IMAGE_MGR(gegl_image_mgr_instance());
}

static void
test_copy_teardown(Test *test)
{
  g_object_unref(source);
  g_object_unref(gray_dest);
  g_object_unref(rgb_dest);
  g_object_unref(simple_image_man);
}

Test *
create_copy_test()
{
  Test* t = ct_create("GeglCopyTest");

  g_assert(ct_addSetUp(t, test_copy_setup));
  g_assert(ct_addTearDown(t, test_copy_teardown));

  g_assert(ct_addTestFun(t, test_copy_g_object_new));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_rgb_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_null_dest_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_gray_null_dest_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_gray_apply));

  return t; 
}
