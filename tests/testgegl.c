#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

extern Test * create_add_test_rgb_float();
extern Test * create_add_test_rgb_uint8();
extern Test * create_bfs_visitor_test();
extern Test * create_channel_value_test();
extern Test * create_channel_float_value_test();
extern Test * create_channel_uint8_value_test();
extern Test * create_check_test();
extern Test * create_color_model_test();
extern Test * create_color_space_test();
extern Test * create_color_test_rgb_float();
extern Test * create_color_test_rgb_uint8();
extern Test * create_copy_test_rgb_float();
extern Test * create_copy_test_rgb_uint8();
extern Test * create_dump_visitor_test();
extern Test * create_channel_space_test();
extern Test * create_buffer_test();
extern Test * create_dfs_visitor_test();
extern Test * create_fade_test_rgb_float();
extern Test * create_fade_test_rgb_uint8();
extern Test * create_graph_apply_test_rgb_float();
extern Test * create_graph_node_test();
extern Test * create_haverect_test();
extern Test * create_i_add_test_rgb_float();
extern Test * create_i_add_test_rgb_uint8();
extern Test * create_i_mult_test_rgb_float();
extern Test * create_i_mult_test_rgb_uint8();
extern Test * create_image_op_test();
extern Test * create_image_iterator_test();
extern Test * create_image_test();
extern Test * create_mult_test_rgb_float();
extern Test * create_mult_test_rgb_uint8();
extern Test * create_multiply_test_rgb_float();
extern Test * create_needrect_test();
extern Test * create_node_test();
extern Test * create_object_test();
extern Test * create_op_test();
extern Test * create_pipe_test();
extern Test * create_pixel_rgb_float_value_test();
extern Test * create_pixel_rgba_float_value_test();
extern Test * create_pixel_rgb_uint8_value_test();
extern Test * create_point_op_test();
extern Test * create_print_test();
extern Test * create_simple_tree_test_rgb_float();
extern Test * create_storage_test();
extern Test * create_tile_test();
extern Test * create_value_image_test();
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
    cs_addTest(suite, create_add_test_rgb_float());
    cs_addTest(suite, create_add_test_rgb_uint8());
    cs_addTest(suite, create_bfs_visitor_test());
    cs_addTest(suite, create_channel_value_test());
    cs_addTest(suite, create_channel_float_value_test());
    cs_addTest(suite, create_channel_uint8_value_test());
    cs_addTest(suite, create_check_test());
    cs_addTest(suite, create_color_test_rgb_uint8());
    cs_addTest(suite, create_color_test_rgb_float());
    cs_addTest(suite, create_color_model_test());
    cs_addTest(suite, create_color_space_test());
    cs_addTest(suite, create_channel_space_test());
    cs_addTest(suite, create_buffer_test());
    cs_addTest(suite, create_dfs_visitor_test());
    cs_addTest(suite, create_dump_visitor_test());
    cs_addTest(suite, create_copy_test_rgb_float());
    cs_addTest(suite, create_copy_test_rgb_uint8());
    cs_addTest(suite, create_fade_test_rgb_float());
    cs_addTest(suite, create_fade_test_rgb_uint8());
    cs_addTest(suite, create_graph_apply_test_rgb_float());
    cs_addTest(suite, create_graph_node_test());
    cs_addTest(suite, create_haverect_test());
    cs_addTest(suite, create_i_add_test_rgb_float());
    cs_addTest(suite, create_i_add_test_rgb_uint8());
    cs_addTest(suite, create_i_mult_test_rgb_float());
    cs_addTest(suite, create_i_mult_test_rgb_uint8());
    cs_addTest(suite, create_image_op_test());
    cs_addTest(suite, create_image_iterator_test());
    cs_addTest(suite, create_image_test());
    cs_addTest(suite, create_mult_test_rgb_float());
    cs_addTest(suite, create_mult_test_rgb_uint8());
    cs_addTest(suite, create_multiply_test_rgb_float());
    cs_addTest(suite, create_needrect_test());
    cs_addTest(suite, create_node_test());
    cs_addTest(suite, create_object_test());
    cs_addTest(suite, create_op_test());
    cs_addTest(suite, create_pipe_test());
    cs_addTest(suite, create_pixel_rgb_float_value_test());
    cs_addTest(suite, create_pixel_rgba_float_value_test());
    cs_addTest(suite, create_pixel_rgb_uint8_value_test());
    cs_addTest(suite, create_point_op_test());
    cs_addTest(suite, create_print_test());
    cs_addTest(suite, create_simple_tree_test_rgb_float());
    cs_addTest(suite, create_storage_test());
    cs_addTest(suite, create_tile_test());
    cs_addTest(suite, create_value_image_test());
    cs_addTest(suite, create_value_test());
#endif

    cs_setStream(suite, stdout);
    cs_run(suite);
    cs_report(suite);
    cs_destroy(suite, TRUE);
  }

  return 0;
}
