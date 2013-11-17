#include <gegl.h> 

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode *graph, *src, *sink;
  GeglBuffer *buffer = NULL;

  gegl_init (&argc, &argv);

  if (argc != 3)
    {
      g_print ("GEGL based image conversion tool\n");
      g_print ("Usage: %s <imageA> <imageB>\n", argv[0]);
      return 1;
    }

  graph = gegl_node_new ();
  src   = gegl_node_new_child (graph,
                               "operation", "gegl:load",
                               "path", argv[1],
                                NULL);
  sink  = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-sink",
                               "buffer", &buffer,
                                NULL);

  gegl_node_link_many (src, sink, NULL);

  gegl_node_process (sink);
  g_object_unref (graph);

  /* FIXME: Currently a buffer will always be returned, even if the source is missing */
  if (!buffer)
    {
      g_print ("Failed to open %s\n", argv[1]);
      return 1;
    }

  graph = gegl_node_new ();
  src   = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer", buffer,
                                NULL);
  sink  = gegl_node_new_child (graph,
                               "operation", "gegl:save",
                               "path", argv[2],
                                NULL);

  gegl_node_link_many (src, sink, NULL);

  gegl_node_process (sink);
  g_object_unref (graph);

  g_object_unref (buffer); 
  gegl_exit ();
  return 0;
}
