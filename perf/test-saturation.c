#include "test-common.h"

void saturation(GeglBuffer *buffer);

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGB float"));
  bench ("saturation (RGB)", buffer, &saturation);
  g_object_unref (buffer);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));
  bench ("saturation (RGBA)", buffer, &saturation);
  g_object_unref (buffer);

  buffer = test_buffer (1024, 1024, babl_format ("CIE Lab float"));
  bench ("saturation (CIE Lab)", buffer, &saturation);
  g_object_unref (buffer);

  buffer = test_buffer (1024, 1024, babl_format ("CIE Lab alpha float"));
  bench ("saturation (CIE Lab alpha)", buffer, &saturation);
  g_object_unref (buffer);

  buffer = test_buffer (1024, 1024, babl_format ("CIE LCH(ab) float"));
  bench ("saturation (CIE LCH(ab))", buffer, &saturation);
  g_object_unref (buffer);

  buffer = test_buffer (1024, 1024, babl_format ("CIE LCH(ab) alpha float"));
  bench ("saturation (CIE LCH(ab) alpha)", buffer, &saturation);
  g_object_unref (buffer);

  gegl_exit ();
  return 0;
}

void saturation(GeglBuffer *buffer)
{
  GeglBuffer *buffer2;
  GeglNode   *gegl, *source, *saturation, *sink;

  gegl = gegl_node_new ();
  source = gegl_node_new_child (gegl, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  saturation = gegl_node_new_child (gegl, "operation", "gegl:saturation", "scale", 1.25, NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink", "buffer", &buffer2, NULL);

  gegl_node_link_many (source, saturation, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);
  g_object_unref (buffer2);
}
