#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *source, *rotate, *sink;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate", "degrees", 4.0, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, rotate, sink, NULL);

  test_start ();
  gegl_node_process (sink);
  test_end ("rotate",  gegl_buffer_get_pixel_count (buffer) * 16);

  g_object_unref (gegl);

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate", "degrees", 4.0, "sampler", GEGL_SAMPLER_NEAREST, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, rotate, sink, NULL);

  test_start ();
  gegl_node_process (sink);
  test_end ("rotate-nearest",  gegl_buffer_get_pixel_count (buffer) * 16);

  g_object_unref (gegl);

  return 0;
}
