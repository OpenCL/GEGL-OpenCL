#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

GeglColorModel * color_model;
#define SAMPLED_IMAGE_WIDTH 10 
#define SAMPLED_IMAGE_HEIGHT 20 

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
test_sampled_image_compute_have_rect(Test *test)
{
  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                 "width", SAMPLED_IMAGE_WIDTH, 
                                                 "height", SAMPLED_IMAGE_HEIGHT, 
                                                 "colormodel", color_model,
                                                  NULL);  
  {
    GeglRect have_rect;
    gegl_image_compute_have_rect(GEGL_IMAGE(sampled_image),
                                    &have_rect,
                                    NULL);

    ct_test(test, SAMPLED_IMAGE_WIDTH == have_rect.w);
    ct_test(test, SAMPLED_IMAGE_HEIGHT == have_rect.h);
    ct_test(test, 0 == have_rect.x);
    ct_test(test, 0 == have_rect.y);
  }

  g_object_unref(sampled_image);
}

static void
test_sampled_image_compute_result_rect(Test *test)
{
  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                                 "width", SAMPLED_IMAGE_WIDTH, 
                                                 "height", SAMPLED_IMAGE_HEIGHT, 
                                                 "colormodel", color_model,
                                                  NULL);  
  {
    GeglRect have_rect;
    GeglRect need_rect;
    GeglRect result_rect;

    gegl_rect_set(&have_rect, 0,0,SAMPLED_IMAGE_WIDTH, SAMPLED_IMAGE_HEIGHT);
    gegl_rect_set(&need_rect, SAMPLED_IMAGE_WIDTH /4,
                               SAMPLED_IMAGE_HEIGHT/4,
                               SAMPLED_IMAGE_WIDTH /2, 
                               SAMPLED_IMAGE_HEIGHT/2);

    gegl_image_compute_result_rect(GEGL_IMAGE(sampled_image),
                                      &result_rect,
                                      &need_rect,
                                      &have_rect);

    ct_test(test, (SAMPLED_IMAGE_WIDTH/2) == result_rect.w);
    ct_test(test, (SAMPLED_IMAGE_HEIGHT/2) == result_rect.h);
    ct_test(test, (SAMPLED_IMAGE_WIDTH/4) == result_rect.x);
    ct_test(test, (SAMPLED_IMAGE_HEIGHT/4) == result_rect.y);
  }

  g_object_unref(sampled_image);
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
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_new));
  g_assert(ct_addTestFun(t, test_sampled_image_g_object_get));
  g_assert(ct_addTestFun(t, test_sampled_image_compute_have_rect));
  g_assert(ct_addTestFun(t, test_sampled_image_compute_result_rect));

  return t; 
}
