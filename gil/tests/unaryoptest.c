#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_unary_op_g_object_new(Test *test)
{
  {
    GilUnaryOp * unary_op = g_object_new (GIL_TYPE_UNARY_OP, NULL);  

    ct_test(test, unary_op != NULL);
    ct_test(test, GIL_IS_UNARY_OP(unary_op));
    ct_test(test, g_type_parent(GIL_TYPE_UNARY_OP) == GIL_TYPE_EXPRESSION);
    ct_test(test, !strcmp("GilUnaryOp", g_type_name(GIL_TYPE_UNARY_OP)));

    g_object_unref(unary_op);
  }

  { 
    /* -A */
    GilVariable * variable = g_object_new(GIL_TYPE_VARIABLE, 
                                          "name", "A", 
                                          NULL); 
    GilUnaryOp * unary_op = g_object_new (GIL_TYPE_UNARY_OP, 
                                          "op", GIL_UNARY_MINUS, 
                                          "operand", variable,
                                          NULL);  
    g_object_unref(unary_op);
    g_object_unref(variable);
  }
}

static void
test_unary_op_g_object_get(Test *test)
{
  {
    GilUnaryOp * unary_op = g_object_new (GIL_TYPE_UNARY_OP, "op", GIL_UNARY_PLUS, NULL);  
    ct_test(test, GIL_UNARY_PLUS == gil_unary_op_get_op(unary_op));
    g_object_unref(unary_op);
  }

  {
    GilUnaryOp * unary_op = g_object_new (GIL_TYPE_UNARY_OP, "op", GIL_UNARY_MINUS, NULL);  
    ct_test(test, GIL_UNARY_MINUS == gil_unary_op_get_op(unary_op));
    g_object_unref(unary_op);
  }

  {
    GilUnaryOp * unary_op = g_object_new (GIL_TYPE_UNARY_OP, "op", GIL_UNARY_NEG, NULL);  
    ct_test(test, GIL_UNARY_NEG == gil_unary_op_get_op(unary_op));
    g_object_unref(unary_op);
  }
}

static void
test_unary_op_g_object_set(Test *test)
{
}

static void
unary_op_setup(Test *test)
{
}

static void
unary_op_teardown(Test *test)
{
}

Test *
create_unary_op_test()
{
  Test *t = ct_create("GilUnaryOpTest");

  g_assert(ct_addSetUp(t, unary_op_setup));
  g_assert(ct_addTearDown(t, unary_op_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_unary_op_g_object_new));
  g_assert(ct_addTestFun(t, test_unary_op_g_object_get));
  g_assert(ct_addTestFun(t, test_unary_op_g_object_set));
#endif

  return t;
}
