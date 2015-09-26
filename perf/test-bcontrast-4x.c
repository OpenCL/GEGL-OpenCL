#include "test-common.h"

void bcontrast4x(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (2048, 1024, babl_format ("RGBA float"));
  bench("bcontrast_4x", buffer, &bcontrast4x);

  return 0;
}

void bcontrast4x(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *node1, *node2, *node3, *node4, *sink;

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
