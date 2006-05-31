#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

extern Test * create_bfs_pad_visitor_test();
extern Test * create_graph_test();
extern Test * create_dfs_pad_visitor_test();
extern Test * create_dfs_node_visitor_test();
extern Test * create_mock_image_operation_test();
extern Test * create_mock_operation_0_1_test();
extern Test * create_mock_operation_1_1_test();
extern Test * create_mock_operation_1_2_test();
extern Test * create_mock_operation_2_1_test();
extern Test * create_mock_operation_2_2_test();
extern Test * create_node_connections_test();
extern Test * create_pad_connections_test();
extern Test * create_update_pad_test();

extern void gegl_tests_init_types (void);

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
  g_log_default_handler (log_domain, log_level, message, user_data);
  g_on_error_stack_trace ("testgegl");
}

int
main (int argc, char *argv[])
{

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                                                  G_LOG_LEVEL_WARNING |
                                                  G_LOG_LEVEL_CRITICAL);
  g_type_init_with_debug_flags (G_TYPE_DEBUG_OBJECTS);

  gegl_init(&argc, &argv);

  g_log_set_default_handler (log_handler, NULL);

  gegl_tests_init_types ();

  {
    Suite *suite = cs_create("GeglTestSuite");

    cs_addTest(suite, create_bfs_pad_visitor_test());
    cs_addTest(suite, create_graph_test());
    cs_addTest(suite, create_dfs_pad_visitor_test());
    cs_addTest(suite, create_dfs_node_visitor_test());
    cs_addTest(suite, create_mock_image_operation_test());
    cs_addTest(suite, create_mock_operation_0_1_test());
    cs_addTest(suite, create_mock_operation_1_1_test());
    cs_addTest(suite, create_mock_operation_1_2_test());
    cs_addTest(suite, create_mock_operation_2_1_test());
    cs_addTest(suite, create_mock_operation_2_2_test());
    cs_addTest(suite, create_node_connections_test());
    cs_addTest(suite, create_pad_connections_test());
    cs_addTest(suite, create_update_pad_test());
    cs_setStream(suite, stdout);
    cs_run(suite);
    cs_report(suite);
    cs_destroy(suite, TRUE);
  }

  return 0;
}
