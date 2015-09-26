#include "test-common.h"

void rotate(GeglBuffer *buffer);
void rotate_nearest(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));
  bench ("rotate", buffer, &rotate);
  bench ("rotate-nearest", buffer, &rotate_nearest);

  return 0;
}

void rotate(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *rotate, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate", "degrees", 4.0, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, rotate, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}

void rotate_nearest(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *rotate, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate", "degrees", 4.0, "sampler", GEGL_SAMPLER_NEAREST, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, rotate, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
