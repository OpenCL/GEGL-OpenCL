#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define IMAGE_OP_WIDTH 1 
#define IMAGE_OP_HEIGHT 1 

#define R0 80 
#define G0 160 
#define B0 240 

#define R1 30 
#define G1 50 
#define B1 70  

static GeglOp *source0, *source1;

static void
test_i_add_g_object_new(Test *test)
{
  {
    GeglOp * i_add = g_object_new (GEGL_TYPE_I_ADD, NULL);  

    ct_test(test, GEGL_IS_I_ADD(i_add));
    ct_test(test, g_type_parent(GEGL_TYPE_BINARY) == GEGL_TYPE_POINT_OP);
    ct_test(test, !strcmp("GeglIAdd", g_type_name(GEGL_TYPE_I_ADD)));

    g_object_unref(i_add);
  }

  {
    GeglOp * i_add =  g_object_new (GEGL_TYPE_I_ADD, NULL);  

    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(i_add), 0));
    ct_test(test, NULL == gegl_node_get_source(GEGL_NODE(i_add), 1));
    ct_test(test, 3 == gegl_node_get_num_inputs(GEGL_NODE(i_add)));
    ct_test(test, 1 == gegl_node_get_num_outputs(GEGL_NODE(i_add)));

    g_object_unref(i_add);
  }
}

static void
test_i_add_properties(Test *test)
{
  {
    GeglOp * i_add = g_object_new (GEGL_TYPE_I_ADD, 
                                   "source-0", source0,
                                   "source-1", source1,
                                   NULL);  

    ct_test(test, source0 == (GeglOp*)gegl_node_get_source(GEGL_NODE(i_add), 0));
    ct_test(test, source1 == (GeglOp*)gegl_node_get_source(GEGL_NODE(i_add), 1));

    g_object_unref(i_add);
  }
}

static void
test_i_add_apply(Test *test)
{
  {
    guint8 r,g,b;

    /* 
       i_add = source0 + source1 
       (110,210,255) = (80,160,240) + (30,50,70)
    */

    GeglOp * i_add = g_object_new (GEGL_TYPE_I_ADD, 
                                   "source-0", source0,
                                   "source-1", source1,
                                   NULL);  

    gegl_op_apply(i_add); 

    r = CLAMP(R0 + R1, 0, 255); 
    g = CLAMP(G0 + G1, 0 ,255); 
    b = CLAMP(B0 + B1, 0 ,255); 

    ct_test(test, testutils_check_rgb_uint8(GEGL_IMAGE_OP(i_add), r, g, b));
    g_object_unref(i_add);
  }
}

static void
i_add_test_setup(Test *test)
{
  GeglColor *color0; 
  GeglColor *color1; 

  color0 = g_object_new(GEGL_TYPE_COLOR, 
                        "rgb-float", R0/255.0, G0/255.0, B0/255.0, 
                        NULL);
  source0 = g_object_new(GEGL_TYPE_FILL, 
                         "width", IMAGE_OP_WIDTH, 
                         "height", IMAGE_OP_HEIGHT, 
                         "fill-color", color0,
                         "image-data-type", "rgb-uint8",
                         NULL); 

  color1 = g_object_new(GEGL_TYPE_COLOR, 
                        "rgb-float", R1/255.0, G1/255.0, B1/255.0, 
                        NULL);
  source1 = g_object_new(GEGL_TYPE_FILL, 
                         "width", IMAGE_OP_WIDTH, 
                         "height", IMAGE_OP_HEIGHT, 
                         "fill-color", color1,
                         "image-data-type", "rgb-uint8",
                         NULL); 
  g_object_unref(color0);
  g_object_unref(color1);
}

static void
i_add_test_teardown(Test *test)
{
  g_object_unref(source0);
  g_object_unref(source1);
}

Test *
create_i_add_test_uint8()
{
  Test* t = ct_create("GeglIAddTestUint8");

  g_assert(ct_addSetUp(t, i_add_test_setup));
  g_assert(ct_addTearDown(t, i_add_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_i_add_g_object_new));
  g_assert(ct_addTestFun(t, test_i_add_properties));
  g_assert(ct_addTestFun(t, test_i_add_apply));
#endif

  return t; 
}
