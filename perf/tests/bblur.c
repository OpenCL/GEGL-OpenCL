#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *sink;

  gegl_init (&argc, &argv);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));

  gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &buffer2, NULL,
                            gegl_node ("gegl:box-blur", "radius", 2.0, NULL,
                            gegl_node ("gegl:buffer-source", "buffer", buffer, NULL))));

  test_start ();
  gegl_node_process (sink);
  test_end ("box-blur", gegl_buffer_get_pixel_count (buffer) * 16);
  g_object_unref (gegl);
  g_object_unref (buffer);

  return 0;
}
