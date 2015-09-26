#include "test-common.h"

void blur(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init(&argc, &argv);

  buffer = test_buffer(1024, 1024, babl_format("RGBA float"));
  bench("gaussian-blur", buffer, &blur);
}

void blur(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *node, *sink;

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
