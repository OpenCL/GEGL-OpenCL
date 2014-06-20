#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *source, *node, *sink;
  gint i;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));

#define ITERATIONS 16
  test_start ();
  for (i=0;i< ITERATIONS;i++)
    {
      gegl = gegl_node_new ();
      source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
      node = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                       "std-dev-x", 0.5,
                                       "std-dev-y", 0.5,
                                       NULL);
      sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

      gegl_node_link_many (source, node, sink, NULL);
      gegl_node_process (sink);
      g_object_unref (gegl);
      g_object_unref (buffer2);
    }
  test_end ("gaussian-blur", gegl_buffer_get_pixel_count (buffer) * 16 * ITERATIONS);

  return 0;
}
