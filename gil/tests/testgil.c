#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gil.h"
#include "ctest.h"
#include "csuite.h"

extern Test * create_bfs_visitor_test();
extern Test * create_binary_op_test();
extern Test * create_block_test();
extern Test * create_constant_test();
extern Test * create_dfs_visitor_test();
extern Test * create_expr_statement_test();
extern Test * create_node_test();
extern Test * create_unary_op_test();
extern Test * create_variable_test();

int
main (int argc, char *argv[])
{  
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                                                 G_LOG_LEVEL_WARNING | 
                                                 G_LOG_LEVEL_CRITICAL);
  g_type_init_with_debug_flags (G_TYPE_DEBUG_OBJECTS);

  {
    Suite *suite = cs_create("GilTestSuite");

#if 1 
    cs_addTest(suite, create_bfs_visitor_test());
    cs_addTest(suite, create_binary_op_test());
    cs_addTest(suite, create_block_test());
    cs_addTest(suite, create_constant_test());
    cs_addTest(suite, create_dfs_visitor_test());
    cs_addTest(suite, create_expr_statement_test());
    cs_addTest(suite, create_node_test());
    cs_addTest(suite, create_unary_op_test());
    cs_addTest(suite, create_variable_test());
#endif

    cs_setStream(suite, stdout);
    cs_run(suite);
    cs_report(suite);
    cs_destroy(suite, TRUE);
  }

  return 0;
}
