#include "test-common.h"

void unsharpmask(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (2048, 1024, babl_format ("RGBA float"));
  bench("unsharp-mask", buffer, &unsharpmask);

  return 0;
}

void unsharpmask(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *node, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  node = gegl_node_new_child (gegl, "operation", "gegl:unsharp-mask",
                                       "std-dev", 3.1,
                                       "scale", 1.2,
                                       NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, node, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
