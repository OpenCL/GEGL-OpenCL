#include "test-common.h"

void scale(GeglBuffer *buffer);
void scale_nearest(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));
  bench ("scale", buffer, &scale);
  bench ("scale-nearest", buffer, &scale_nearest);
  g_object_unref (buffer);

  gegl_exit ();
  return 0;
}

void scale(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *scale, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  scale = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", "x", 0.4, "y", 0.4, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, scale, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}

void scale_nearest(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *scale, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  scale = gegl_node_new_child (gegl,
                               "operation", "gegl:scale-ratio",
                               "x", 0.4,
                               "y", 0.4,
                               "sampler", GEGL_SAMPLER_NEAREST,
                               NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, scale, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
