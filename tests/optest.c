#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-op.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_op_g_object_new(Test *test)
{
  {
    GeglOp * op = g_object_new (GEGL_TYPE_MOCK_OP, 
                                "num_inputs", 1,
                                NULL);  

    ct_test(test, op != NULL);

    ct_test(test, GEGL_IS_OP(op));
    ct_test(test, g_type_parent(GEGL_TYPE_OP) == GEGL_TYPE_NODE);
    ct_test(test, !strcmp("GeglOp", g_type_name(GEGL_TYPE_OP)));

    ct_test(test, GEGL_IS_MOCK_OP(op));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_OP) == GEGL_TYPE_OP);
    ct_test(test, !strcmp("GeglMockOp", g_type_name(GEGL_TYPE_MOCK_OP)));

    g_object_unref(op);
  }
}

static void
test_op_g_object_new_source0(Test *test)
{
  {
    GeglNode *B, *A;
    B = g_object_new (GEGL_TYPE_MOCK_OP, 
                      "num_outputs", 1,
                      NULL);  
    A = g_object_new (GEGL_TYPE_MOCK_OP, 
                      "num_inputs", 1,
                      "input-0", B, 
                      NULL);  

    ct_test(test, 1 == gegl_node_get_num_outputs(B)); 
    ct_test(test, 0 == gegl_node_get_num_inputs(B)); 
    ct_test(test, 1 == gegl_node_get_num_sinks(B)); 
    ct_test(test, 0 == gegl_node_get_num_outputs(A)); 
    ct_test(test, 1 == gegl_node_get_num_inputs(A)); 


    g_object_unref(A);
    g_object_unref(B);
  }
}

static void
op_test_setup(Test *test)
{
}

static void
op_test_teardown(Test *test)
{
}

Test *
create_op_test()
{
  Test* t = ct_create("GeglOpTest");

  g_assert(ct_addSetUp(t, op_test_setup));
  g_assert(ct_addTearDown(t, op_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_op_g_object_new));
  g_assert(ct_addTestFun(t, test_op_g_object_new_source0));
#endif
                                     
  return t; 
}
