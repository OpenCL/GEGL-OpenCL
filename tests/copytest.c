#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglOp * source;
static GeglOp * gray_dest;
static GeglOp * rgb_dest;

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
                                    "source", source,
                                    NULL);  

    ct_test(t, copy != NULL);
    ct_test(t, 1 == gegl_node_get_num_inputs(GEGL_NODE(copy)));
    ct_test(t, 1 == gegl_node_get_num_outputs(GEGL_NODE(copy)));
    ct_test(t, source == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(copy),0));

    g_object_unref(copy);
  }
}

static void
test_copy_rgb_to_rgb_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                         "source", source,
                                         NULL);

    gegl_op_apply_image(copy, GEGL_OP(rgb_dest), NULL); 

    ct_test(t, testutils_check_pixel(GEGL_IMAGE(rgb_dest), "RgbFloat", 
                                     .1 , .2 , .3, 0));  

    g_object_unref(copy);
  }
}

static void
test_copy_rgb_to_null_dest_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "source", source,
                                NULL);

    gegl_op_apply_image(copy, NULL, NULL); 

    ct_test(t, testutils_check_pixel(GEGL_IMAGE(copy), "RgbFloat", 
                                     .1 , .2 , .3, 0));  

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
                                "source", source,
                                NULL);

    gegl_op_apply_image(copy, NULL, NULL); 

    ct_test(t, testutils_check_pixel(GEGL_IMAGE(copy), "GrayFloat", 
                                     .3*.1 + .59*.2 + .11*.3, 0,0,0));  

    g_object_unref(copy);
  }
}


static void
test_copy_rgb_to_gray_apply(Test *t)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "source", source,
                                NULL);

    gegl_op_apply_image(copy, GEGL_OP(gray_dest), NULL); 

    /* .3*r + .59*g + .11*b */

    ct_test(t, testutils_check_pixel(GEGL_IMAGE(gray_dest), "GrayFloat", 
                                     .3*.1 + .59*.2 + .11*.3, 0,0,0));  

    g_object_unref(copy);
  }
}

static void
copy_test_setup(Test *t)
{
  /* source is a rgb float image */
  source = testutils_sampled_image("RgbFloat",
                                         SAMPLED_IMAGE_WIDTH, SAMPLED_IMAGE_HEIGHT, 
                                         .1, .2, .3, 0);

  {
    /* dest is a gray float image */
    GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");
    gray_dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                              "colormodel", gray_float,
                              "width", SAMPLED_IMAGE_WIDTH, 
                              "height", SAMPLED_IMAGE_HEIGHT,
                              NULL);  
  }

  {
    /* dest is a gray float image */
    GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
    rgb_dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                             "colormodel", rgb_float,
                             "width", SAMPLED_IMAGE_WIDTH, 
                             "height", SAMPLED_IMAGE_HEIGHT,
                             NULL);  
  }
}

static void
copy_test_teardown(Test *test)
{
  g_object_unref(source);
  g_object_unref(gray_dest);
  g_object_unref(rgb_dest);
}

Test *
create_copy_test()
{
  Test* t = ct_create("GeglCopyTest");

  g_assert(ct_addSetUp(t, copy_test_setup));
  g_assert(ct_addTearDown(t, copy_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_copy_g_object_new));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_rgb_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_null_dest_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_gray_null_dest_apply));
  g_assert(ct_addTestFun(t, test_copy_rgb_to_gray_apply));
#endif

  return t; 
}
