#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_expr_statement_g_object_new(Test *test)
{
  {
    GilExprStatement * expr_statement = g_object_new (GIL_TYPE_EXPR_STATEMENT, NULL);  

    ct_test(test, expr_statement != NULL);
    ct_test(test, GIL_IS_EXPR_STATEMENT(expr_statement));
    ct_test(test, g_type_parent(GIL_TYPE_EXPR_STATEMENT) == GIL_TYPE_STATEMENT);
    ct_test(test, !strcmp("GilExprStatement", g_type_name(GIL_TYPE_EXPR_STATEMENT)));

    g_object_unref(expr_statement);
  }

  {
    /* A = B + 1.5 */
    GilConstant * constant = g_object_new(GIL_TYPE_CONSTANT, 
                                          "type", GIL_FLOAT, 
                                          "float", 1.5, 
                                          NULL); 
    GilVariable * B = g_object_new(GIL_TYPE_VARIABLE, 
                                   "name", "B", 
                                   NULL); 

    GilBinaryOp * bin_op = g_object_new (GIL_TYPE_BINARY_OP, 
                                         "op", GIL_PLUS, 
                                         "left_operand", B,
                                         "right_operand", constant,
                                         NULL);  

    GilVariable * A = g_object_new(GIL_TYPE_VARIABLE, 
                                   "name", "A",
                                   NULL);

    GilExprStatement * expr_statement = g_object_new (GIL_TYPE_EXPR_STATEMENT, 
                                                      "left_expr", A,
                                                      "right_expr", bin_op,
                                                      NULL);  

    g_object_unref(expr_statement);
    g_object_unref(A);
    g_object_unref(bin_op);
    g_object_unref(B);
    g_object_unref(constant);
  }
}

static void
test_expr_statement_g_object_get(Test *test)
{
}


static void
test_expr_statement_g_object_set(Test *test)
{
}

static void
expr_statement_setup(Test *test)
{
}

static void
expr_statement_teardown(Test *test)
{
}

Test *
create_expr_statement_test()
{
  Test *t = ct_create("GilExprStatementTest");

  g_assert(ct_addSetUp(t, expr_statement_setup));
  g_assert(ct_addTearDown(t, expr_statement_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_expr_statement_g_object_new));
  g_assert(ct_addTestFun(t, test_expr_statement_g_object_get));
  g_assert(ct_addTestFun(t, test_expr_statement_g_object_set));
#endif

  return t;
}
