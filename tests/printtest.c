#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

#define SAMPLED_IMAGE_WIDTH 5 
#define SAMPLED_IMAGE_HEIGHT 5 

static GeglOp * source;
static GeglOp * dest;

static void
test_print_g_object_new(Test *test)
{
  {
    GeglPrint * print = g_object_new (GEGL_TYPE_PRINT, NULL);  

    ct_test(test, print != NULL);
    ct_test(test, GEGL_IS_PRINT(print));
    ct_test(test, g_type_parent(GEGL_TYPE_PRINT) == GEGL_TYPE_STAT_OP);
    ct_test(test, !strcmp("GeglPrint", g_type_name(GEGL_TYPE_PRINT)));

    g_object_unref(print);
  }
}

static void
test_print_apply(Test *test)
{
  GeglOp *print = g_object_new(GEGL_TYPE_PRINT,
                               "input", source,
                               NULL);

  gegl_op_apply(print); 

  g_object_unref(print);
}

static void
print_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
  source = testutils_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                             SAMPLED_IMAGE_HEIGHT, 
                                             .1, .2, .3);

  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

  g_object_unref(rgb_float);
}

static void
print_test_teardown(Test *test)
{
  g_object_unref(source);
  g_object_unref(dest);
}

Test *
create_print_test()
{
  Test* t = ct_create("GeglPrintTest");

  g_assert(ct_addSetUp(t, print_test_setup));
  g_assert(ct_addTearDown(t, print_test_teardown));
  g_assert(ct_addTestFun(t, test_print_g_object_new));
  g_assert(ct_addTestFun(t, test_print_apply));

  return t; 
}
