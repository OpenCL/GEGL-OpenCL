/* File: copytest-rbg-float.c
 * Author: Daniel S. Rogers
 * Date: 21 February, 2003
 *Derived from fadetest-rbg-float.c
 */

#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

static GeglOp * source;

static void
test_copy_g_object_new(Test *test)
{
  {
    GeglCopy * copy = g_object_new (GEGL_TYPE_COPY, NULL);  

    ct_test(test, copy != NULL);
    ct_test(test, GEGL_IS_COPY(copy));
    ct_test(test, g_type_parent(GEGL_TYPE_COPY) == GEGL_TYPE_UNARY);
    ct_test(test, !strcmp("GeglCopy", g_type_name(GEGL_TYPE_COPY)));

    g_object_unref(copy);
  }
}

static void
test_copy_g_object_properties(Test *test)
{
  {
    GeglCopy * copy = g_object_new (GEGL_TYPE_COPY, 
                                    "input", 0, source,
                                     NULL);  

    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(copy)));
    ct_test(test, source == (GeglOp*)gegl_node_get_source(GEGL_NODE(copy), 0));

    g_object_unref(copy);
  }

  {
    GeglCopy * copy = g_object_new (GEGL_TYPE_COPY, NULL);  

    g_object_set(copy, NULL);

    g_object_unref(copy);
  }

  {
    GeglCopy * copy = g_object_new (GEGL_TYPE_COPY, 
                                    NULL);  

    g_object_get(copy, NULL);

    g_object_unref(copy);
  }
}

static void
test_copy_apply(Test *test)
{
  {
    GeglOp *copy = g_object_new(GEGL_TYPE_COPY,
                                "input", 0, source,
                                NULL);

    gegl_op_apply(copy); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(copy), 
                                            .1, 
                                            .2, 
                                            .3));  
    g_object_unref(copy);
  }

  {
    GeglOp *copy1 = g_object_new(GEGL_TYPE_COPY,
                                 "input", 0, source,
                                 NULL);

    GeglOp *copy2 = g_object_new(GEGL_TYPE_COPY,
                                 "input", 0, copy1,
                                 NULL);

    gegl_op_apply(copy2); 

    ct_test(test, testutils_check_pixel_rgb_float(GEGL_IMAGE_OP(copy2), 
                                            .1, 
                                            .2, 
                                            .3));  

    g_object_unref(copy1);
    g_object_unref(copy2);
  }
}

static void
copy_test_setup(Test *test)
{
  source = g_object_new(GEGL_TYPE_COLOR, 
                        "width", IMAGE_OP_WIDTH, 
                        "height", IMAGE_OP_HEIGHT, 
                        "pixel-rgb-float",.1, .2, .3, 
                        NULL); 

}

static void
copy_test_teardown(Test *test)
{
  g_object_unref(source);
}

Test *
create_copy_test_rgb_float()
{
  Test* t = ct_create("GeglCopyTestRgbFloat");

  g_assert(ct_addSetUp(t, copy_test_setup));
  g_assert(ct_addTearDown(t, copy_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_copy_g_object_new));
  g_assert(ct_addTestFun(t, test_copy_g_object_properties));
  g_assert(ct_addTestFun(t, test_copy_apply));
#endif

  return t; 
}
