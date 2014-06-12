#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglNode   *gegl, *sink;
  gint i;

  gegl_init (&argc, &argv);

  g_object_set (gegl_config (), "chunk-size", 128 * 128, NULL);

  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));

#define ITERATIONS 6
  test_start ();
  for (i=0;i< ITERATIONS;i++)
    {
      gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &buffer2, NULL,
                                gegl_node ("gegl:brightness-contrast", "contrast", 0.2, NULL,
                                gegl_node ("gegl:buffer-source", "buffer", buffer, NULL))));

      gegl_node_process (sink);
      g_object_unref (gegl);
      g_object_unref (buffer2);
    }
  test_end ("bcontrast-minichunk", gegl_buffer_get_pixel_count (buffer) * 16 * ITERATIONS);


  return 0;
}
