#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_binary_op_g_object_new(Test *test)
{
  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, NULL);  

    ct_test(test, binary_op != NULL);
    ct_test(test, GIL_IS_BINARY_OP(binary_op));
    ct_test(test, g_type_parent(GIL_TYPE_BINARY_OP) == GIL_TYPE_EXPRESSION);
    ct_test(test, !strcmp("GilBinaryOp", g_type_name(GIL_TYPE_BINARY_OP)));

    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_PLUS, NULL);  
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_MINUS, NULL);  
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_MULT, NULL);  
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_DIV, NULL);  
    g_object_unref(binary_op);
  }

  {
    /* A + 1.5 */
    GilVariable * variable = g_object_new(GIL_TYPE_VARIABLE, 
                                          "name", "A", 
                                          NULL); 
    GilConstant * constant = g_object_new(GIL_TYPE_CONSTANT, 
                                          "type", GIL_FLOAT, 
                                          "float", 1.5, 
                                          NULL); 
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, 
                                            "op", GIL_PLUS, 
                                            "left_operand", variable,
                                            "right_operand", constant,
                                            NULL);  
    g_object_unref(binary_op);
    g_object_unref(variable);
    g_object_unref(constant);
  }
}

static void
test_binary_op_g_object_get(Test *test)
{
  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_PLUS, NULL);  
    ct_test(test, GIL_PLUS == gil_binary_op_get_op(binary_op));
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_MINUS, NULL);  
    ct_test(test, GIL_MINUS == gil_binary_op_get_op(binary_op));
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_MULT, NULL);  
    ct_test(test, GIL_MULT == gil_binary_op_get_op(binary_op));
    g_object_unref(binary_op);
  }

  {
    GilBinaryOp * binary_op = g_object_new (GIL_TYPE_BINARY_OP, "op", GIL_DIV, NULL);  
    ct_test(test, GIL_DIV == gil_binary_op_get_op(binary_op));
    g_object_unref(binary_op);
  }
}

static void
test_binary_op_g_object_set(Test *test)
{
}

static void
binary_op_setup(Test *test)
{
}

static void
binary_op_teardown(Test *test)
{
}

Test *
create_binary_op_test()
{
  Test *t = ct_create("GilBinaryOpTest");

  g_assert(ct_addSetUp(t, binary_op_setup));
  g_assert(ct_addTearDown(t, binary_op_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_binary_op_g_object_new));
  g_assert(ct_addTestFun(t, test_binary_op_g_object_get));
  g_assert(ct_addTestFun(t, test_binary_op_g_object_set));
#endif

  return t;
}
