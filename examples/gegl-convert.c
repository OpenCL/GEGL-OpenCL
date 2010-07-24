#include <gegl.h> 

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode *graph, *sink;
  GeglBuffer *buffer = NULL;

  gegl_init (&argc, &argv);

  if (argc != 3)
    {
      g_print ("GEGL based image conversion tool\n");
      g_print ("Usage: %s <imageA> <imageB>\n", argv[0]);
      return 1;
    }

  graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &buffer, NULL,
                           gegl_node ("gegl:load", "path", argv[1], NULL)));
  gegl_node_process (sink);
  g_object_unref (graph);
  if (!buffer)
    {
      g_print ("Failed to open %s\n", argv[1]);
      return 1;
    }

  graph = gegl_graph (sink=gegl_node ("gegl:save",
                      "path", argv[2], NULL,
                      gegl_node ("gegl:buffer-source", "buffer", buffer, NULL)));
  gegl_node_process (sink);

  g_object_unref (buffer); 
  g_object_unref (buffer);  /* XXX: why is two unrefs needed here? */
  g_object_unref (graph);
  gegl_exit ();
  return 0;
}
