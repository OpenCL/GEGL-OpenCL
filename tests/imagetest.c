#include "gegl.h"
#include "gegl-mock-image.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

static GeglColorModel * color_model;

static void
test_image_g_object_new(Test *test)
{
  GeglImage * image = g_object_new (GEGL_TYPE_MOCK_IMAGE, NULL);  

  ct_test(test, image != NULL);
  ct_test(test, GEGL_IS_IMAGE(image));
  ct_test(test, g_type_parent(GEGL_TYPE_IMAGE) == GEGL_TYPE_FILTER);
  ct_test(test, !strcmp("GeglImage", g_type_name(GEGL_TYPE_IMAGE)));

  ct_test(test, GEGL_IS_MOCK_IMAGE(image));
  ct_test(test, g_type_parent(GEGL_TYPE_MOCK_IMAGE) == GEGL_TYPE_IMAGE);
  ct_test(test, !strcmp("GeglMockImage", g_type_name(GEGL_TYPE_MOCK_IMAGE)));

  g_object_unref(image);
}

static void
test_image_g_object_get(Test *test)
{
  GeglColorModel *cmodel;
  GeglImage * image = g_object_new (GEGL_TYPE_MOCK_IMAGE, 
                                    "colormodel", color_model,
                                     NULL);  

  g_object_get(image, "colormodel", &cmodel, NULL);

  ct_test(test, color_model == cmodel);

  g_object_unref(cmodel);
  g_object_unref(image);
}

static void
test_image_inputs_outputs(Test *test)
{
  GeglImage * image = g_object_new (GEGL_TYPE_MOCK_IMAGE, 
                                    "colormodel", color_model,
                                     NULL);  

  ct_test(test, 0 == gegl_node_get_num_inputs(GEGL_NODE(image))); 
  ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(image))); 

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
  g_assert(ct_addTestFun(t, test_image_inputs_outputs));

  return t; 
}
