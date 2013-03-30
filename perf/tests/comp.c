#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglBuffer *bufferB;
  GeglNode   *gegl, *sink;
  gint i;

  gegl_init (&argc, &argv);

  bufferB = test_buffer (1024, 1024, babl_format ("RGBA float"));
  buffer = test_buffer (1024, 1024, babl_format ("RGBA float"));

#define ITERATIONS 8
  test_start ();
  for (i=0;i< ITERATIONS;i++)
    {
      gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &buffer2, NULL,
                                gegl_node ("gegl:over", NULL,
                                gegl_node ("gegl:buffer-source", "buffer", buffer, NULL),
                                gegl_node ("gegl:buffer-source", "buffer", bufferB, NULL))));

      gegl_node_process (sink);
      g_object_unref (gegl);
      g_object_unref (buffer2);
    }
  test_end ("over", gegl_buffer_get_pixel_count (bufferB) * ITERATIONS * 16);
  return 0;
}
