#include <stdlib.h>
#include <glib.h>
#include <gegl.h>
#include "misc.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer, *buffer2;
  GeglBuffer *bufferB;
  GeglNode   *gegl, *sink;
  long ticks;
  gint i;
  gchar *infile;
  gchar *infileB;

  gegl_init (&argc, &argv);

  infile = "/home/pippin/images/movie_narrative_charts_large.png";
  infileB = "/home/pippin/images/movieb.png";

  gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &bufferB, NULL,
                            gegl_node ("fooit", NULL,
                            gegl_node ("gegl:png-load", "path",  infileB, NULL))));

  gegl_node_process (sink);
  g_object_unref (gegl);

  g_print (":::%s\n", babl_get_name (gegl_buffer_get_format (bufferB)));
  gegl = gegl_graph (sink = gegl_node ("gegl:buffer-sink", "buffer", &buffer, NULL,
                            gegl_node ("fooit", NULL,
                            gegl_node ("gegl:png-load", "path",  infile, NULL))));

  gegl_node_process (sink);
  g_object_unref (gegl);

  g_print (":::%s\n", babl_get_name (gegl_buffer_get_format (buffer)));

#define ITERATIONS 3
  ticks = gegl_ticks ();
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

  ticks = gegl_ticks ()-ticks;
  g_print ("@ over: %f Mpixels/sec\n", ((1.0 * gegl_buffer_get_width (buffer) * gegl_buffer_get_height (buffer) * ITERATIONS/1000000)/ ( (ticks) / 1000.0) * 1000.0));

  return 0;
}
