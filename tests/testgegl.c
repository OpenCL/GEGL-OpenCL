#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"

extern Test * create_add_test();
extern Test * create_buffer_test();
extern Test * create_color_test();
extern Test * create_color_model_test();
extern Test * create_color_model_rgb_test();
extern Test * create_color_model_rgb_float_test();
extern Test * create_const_mult_test();
extern Test * create_copy_test();
extern Test * create_fill_test();
extern Test * create_filter_test();
extern Test * create_image_test();
extern Test * create_mem_buffer_test();
extern Test * create_mem_image_mgr_test();
extern Test * create_node_test();
extern Test * create_object_test();
extern Test * create_op_test();
extern Test * create_point_op_test();
extern Test * create_print_test();
extern Test * create_sampled_image_test();
extern Test * create_simpletree_test();
extern Test * create_stat_op_test();
extern Test * create_tile_iterator_test();
extern Test * create_tile_mgr_test();
extern Test * create_tile_test();
extern Test * create_value_test();

int
main (int argc, char *argv[])
{  

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                                                 G_LOG_LEVEL_WARNING | 
                                                 G_LOG_LEVEL_CRITICAL);
  g_type_init_with_debug_flags (G_TYPE_DEBUG_OBJECTS);

  gegl_init(&argc, &argv);

  {
    Suite *suite = cs_create("GeglTestSuite");

#if 1 
    cs_addTest(suite, create_add_test());
    cs_addTest(suite, create_buffer_test());
    cs_addTest(suite, create_color_test());
    cs_addTest(suite, create_color_model_test());
    cs_addTest(suite, create_color_model_rgb_test());
    cs_addTest(suite, create_color_model_rgb_float_test());
    cs_addTest(suite, create_const_mult_test());
    cs_addTest(suite, create_copy_test());
    cs_addTest(suite, create_fill_test());
    cs_addTest(suite, create_filter_test());
    cs_addTest(suite, create_image_test());
    cs_addTest(suite, create_mem_buffer_test());
    cs_addTest(suite, create_node_test());
    cs_addTest(suite, create_object_test());
    cs_addTest(suite, create_op_test());
    cs_addTest(suite, create_point_op_test());
    cs_addTest(suite, create_print_test());
    cs_addTest(suite, create_simpletree_test());
    cs_addTest(suite, create_sampled_image_test());
    cs_addTest(suite, create_tile_test());
    cs_addTest(suite, create_tile_iterator_test());
    cs_addTest(suite, create_tile_mgr_test());
    cs_addTest(suite, create_value_test());
#endif

    cs_setStream(suite, stdout);
    cs_run(suite);
    cs_report(suite);
    cs_destroy(suite, TRUE);
  }

  return 0;
}
