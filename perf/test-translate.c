#include "test-common.h"

void translate_integer(GeglBuffer *buffer);
void translate_nearest(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("R'G'B' u8"));
  bench ("translate-integer", buffer, &translate_integer);
  bench ("translate-nearest", buffer, &translate_nearest);

  return 0;
}

void translate_integer(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *translate, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  translate = gegl_node_new_child (gegl, "operation", "gegl:translate", "x", 10.0, "y", 10.0, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, translate, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}

void translate_nearest(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *translate, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  translate = gegl_node_new_child (gegl,
                                   "operation", "gegl:translate",
                                   "x", 10.5,
                                   "y", 10.5,
                                   "sampler", GEGL_SAMPLER_NEAREST,
                                   NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, translate, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
