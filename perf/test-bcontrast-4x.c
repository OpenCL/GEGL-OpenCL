#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *source, *node1, *node2, *node3, *node4, *sink;
  gint i;

  gegl_init (&argc, &argv);

  buffer = test_buffer (2048, 1024, babl_format ("RGBA float"));

#define ITERATIONS 16
  test_start ();
  for (i=0;i< ITERATIONS;i++)
    {
      gegl = gegl_node_new ();
      source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
      node1 = gegl_node_new_child (gegl, "operation", "gegl:brightness-contrast", "contrast", 0.2, NULL);
      node2 = gegl_node_new_child (gegl, "operation", "gegl:brightness-contrast", "contrast", 0.2, NULL);
      node3 = gegl_node_new_child (gegl, "operation", "gegl:brightness-contrast", "contrast", 0.2, NULL);
      node4 = gegl_node_new_child (gegl, "operation", "gegl:brightness-contrast", "contrast", 0.2, NULL);
      sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

      gegl_node_link_many (source, node1, node2, node3, node4, sink, NULL);
      gegl_node_process (sink);
      g_object_unref (gegl);
      g_object_unref (buffer2);
    }
  test_end ("bcontrast_4x", gegl_buffer_get_pixel_count (buffer) * 16 * ITERATIONS);

  return 0;
}
