#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *sink;
  gint i;

  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  buffer = test_buffer (2048, 2048, babl_format ("RGBA float"));

#define ITERATIONS 4
  test_start ();
  for (i=0;i< ITERATIONS;i++)
  gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &buffer2, NULL,
                            gegl_node ("gegl:brightness-contrast", "contrast", 0.2, NULL,
                            gegl_node ("gegl:brightness-contrast", "contrast", 0.2, NULL,
                            gegl_node ("gegl:brightness-contrast", "contrast", 0.2, NULL,
                            gegl_node ("gegl:brightness-contrast", "contrast", 1.2, NULL,
                            gegl_node ("gegl:buffer-source", "buffer", buffer, NULL)))))));

  gegl_node_process (sink);
  test_end ("passthrough", gegl_buffer_get_pixel_count (buffer2) * 16);
  g_object_unref (gegl);
  g_object_unref (buffer2);

  return 0;
}
