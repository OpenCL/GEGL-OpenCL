#include <gegl.h>

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode *gegl;      /*< Main GEGL graph */
  GeglNode *load,      /*< The image processing nodes */
           *scale,
           *bcontrast,
           *layer,
           *text,
           *dropshadow,
           *save;

  if (argc < 2)
   { 
     g_print ("Usage: %s <input> [output.png]\n\n", argv[0]);
     return -1;
   }

  gegl_init (&argc, &argv);  /* initialize the GEGL library */

  gegl = gegl_graph_new ();  /* instantiate a graph */

  /* create nodes for the graph, with specified operations */
  load       = gegl_graph_create_node (gegl, "load");
  layer      = gegl_graph_create_node (gegl, "layer");
  scale      = gegl_graph_create_node (gegl, "scale");
  bcontrast  = gegl_graph_create_node (gegl, "brightness-contrast");
  text       = gegl_graph_create_node (gegl, "text");
  dropshadow = gegl_graph_create_node (gegl, "dropshadow");
  save       = gegl_graph_create_node (gegl, "png-save"); /* note: the
                                          png-save operation will probably
                                          be removed from future versions
                                          of GEGL, but it is useful for
                                          debugging. */                

  /* link the nodes together */
  gegl_node_link_many (load, scale, bcontrast, layer, save, NULL);
  gegl_node_link (text, dropshadow);
  gegl_node_connect (layer, "aux", dropshadow, "output");

  /* set properties for the nodes */
  gegl_node_set (load, "path",  argv[1], NULL);
  gegl_node_set (scale,
                 "x",  0.5,
                 "y",  0.5, NULL);
  gegl_node_set (bcontrast,
                 "contrast", 3.0, NULL);
  gegl_node_set (layer,
                 "x", 10.0,
                 "y", 10.0, NULL);
  gegl_node_set (text,
                 "string", "Hello World",
                 "size", 20.0,
                 "color", gegl_color_from_string ("rgb(0.1,0.5,0.9)"),
                 NULL);
  gegl_node_set (save,
                 "path", argv[2]?argv[2]:"output.png",
                 NULL);
 
  /* request evaluation of the "output" pad of the png-save op */ 
  gegl_node_apply (save, "output");

  /* free resources used by the graph and the nodes it owns */
  g_object_unref (gegl);

  /* free resources globally used by GEGL */
  gegl_exit ();

  return 0;
}
