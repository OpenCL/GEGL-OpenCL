#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-filter.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

GeglColorModel * color_model;
#define SAMPLED_IMAGE_WIDTH 10 
#define SAMPLED_IMAGE_HEIGHT 5 

static void
test_sampled_image_g_object_new(Test *test)
{
  {
    GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                     "width", SAMPLED_IMAGE_WIDTH, 
                                                     "height", SAMPLED_IMAGE_HEIGHT, 
                                                     "colormodel", color_model,
                                                      NULL);  

    ct_test(test, sampled_image != NULL);
    ct_test(test, GEGL_IS_SAMPLED_IMAGE(sampled_image));
    ct_test(test, g_type_parent(GEGL_TYPE_SAMPLED_IMAGE) == GEGL_TYPE_IMAGE);
    ct_test(test, !strcmp("GeglSampledImage", g_type_name(GEGL_TYPE_SAMPLED_IMAGE)));

    g_object_unref(sampled_image);
  }
}

static void
test_sampled_image_g_object_get(Test *test)
{
  gint width, height;
  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                   "width", SAMPLED_IMAGE_WIDTH, 
                                                   "height", SAMPLED_IMAGE_HEIGHT, 
                                                   "colormodel", color_model,
                                                    NULL);  
  g_object_get(sampled_image, 
               "width", &width, 
               "height", &height, 
               NULL);

  ct_test(test, SAMPLED_IMAGE_WIDTH == width);
  ct_test(test, SAMPLED_IMAGE_HEIGHT == height);

  g_object_unref(sampled_image);
}

static void
test_sampled_image_source_apply(Test *test)
{
  GeglRect roi = {0,0,10,10};

  GeglSampledImage * image0 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                            "width", SAMPLED_IMAGE_WIDTH, 
                                            "height", SAMPLED_IMAGE_HEIGHT, 
                                            "colormodel", color_model,
                                             NULL);  

  GeglSampledImage * image1 = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                            "width", SAMPLED_IMAGE_WIDTH, 
                                            "height", SAMPLED_IMAGE_HEIGHT, 
                                            "colormodel", color_model,
                                             NULL);  

  GeglOp * op = g_object_new (GEGL_TYPE_MOCK_FILTER, 
                              "num_inputs", 2, 
                              "num_outputs", 1, 
                              "source0", image0,
                              "source1", image1,
                              NULL);  

  gegl_op_apply_roi(op, &roi);

  g_object_unref(op);
  g_object_unref(image0);
  g_object_unref(image1);
}

static void
sampled_image_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
}

static void
sampled_image_test_teardown(Test *test)
{
  g_object_unref(color_model);
}

Test *
create_sampled_image_test()
{
  Test* t = ct_create("GeglSampledImageTest");

  g_assert(ct_addSetUp(t, sampled_image_test_setup));
  g_assert(ct_addTearDown(t, sampled_image_test_teardown));
  g_assert(ct_addTestFun(t, test_sampled_image_source_apply));
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_new));
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_get));

  return t; 
}
