#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

static GeglColorModel * color_model;
#define SAMPLED_IMAGE_WIDTH 10 
#define SAMPLED_IMAGE_HEIGHT 20 

static void
test_image_g_object_new(Test *test)
{
  GeglImage * image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                    "width", SAMPLED_IMAGE_WIDTH, 
                                    "height", SAMPLED_IMAGE_HEIGHT, 
                                    "colormodel", color_model,
                                     NULL);  

  ct_test(test, image != NULL);
  ct_test(test, GEGL_IS_IMAGE(image));
  ct_test(test, g_type_parent(GEGL_TYPE_IMAGE) == GEGL_TYPE_OP);
  ct_test(test, !strcmp("GeglImage", g_type_name(GEGL_TYPE_IMAGE)));

  g_object_unref(image);
}

static void
test_image_g_object_get(Test *test)
{
  GeglColorModel *cmodel;
  GeglImage * image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE, 
                                    "width", SAMPLED_IMAGE_WIDTH, 
                                    "height", SAMPLED_IMAGE_HEIGHT, 
                                    "colormodel", color_model,
                                     NULL);  

  g_object_get(image, "colormodel", &cmodel, NULL);

  ct_test(test, color_model == cmodel);

  g_object_unref(cmodel);
  g_object_unref(image);
}

static void
image_test_setup(Test *test)
{
  color_model = gegl_color_model_instance("RgbFloat");
}

static void
image_test_teardown(Test *test)
{
  g_object_unref(color_model);
}

Test *
create_image_test()
{
  Test* t = ct_create("GeglImageTest");

  g_assert(ct_addSetUp(t, image_test_setup));
  g_assert(ct_addTearDown(t, image_test_teardown));
  g_assert(ct_addTestFun(t, test_image_g_object_new));
  g_assert(ct_addTestFun(t, test_image_g_object_get));

  return t; 
}
