#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_WIDTH 5 
#define IMAGE_HEIGHT 5 

#define R0 .1 
#define G0 .2
#define B0 .3 

static GeglOp * source;

static void
test_print_g_object_new(Test *test)
{
  {
    GeglPrint * print = g_object_new (GEGL_TYPE_PRINT, NULL);  

    ct_test(test, print != NULL);
    ct_test(test, GEGL_IS_PRINT(print));
    ct_test(test, g_type_parent(GEGL_TYPE_PRINT) == GEGL_TYPE_PIPE);
    ct_test(test, !strcmp("GeglPrint", g_type_name(GEGL_TYPE_PRINT)));

    g_object_unref(print);
  }
}

static void
test_print_apply(Test *test)
{
  GeglOp *print = g_object_new(GEGL_TYPE_PRINT,
                               "input", 0, source,
                               NULL);

  gegl_op_apply(print); 

  g_object_unref(print);
}

static void
print_test_setup(Test *test)
{
  source = g_object_new(GEGL_TYPE_COLOR, 
                        "width", IMAGE_WIDTH, 
                        "height", IMAGE_HEIGHT, 
                        "pixel-rgb-float", R0, G0, B0, 
                        NULL); 

}

static void
print_test_teardown(Test *test)
{
  g_object_unref(source);
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
