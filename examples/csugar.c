#include <gegl.h>

/* Example file illustrating the syntactic sugar for graph
 * construction in C
 */

gint
main (gint argc,
      gchar **argv)
{
  GeglNode *gegl, *sink;

  if (argc != 4)
    {
      g_print ("Usage: %s <input image> \"string\" <output image>\n\n", argv[0]);
      return -1;
    }

  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  gegl =
    gegl_graph (
    sink = gegl_node ("gegl:png-save", "path", argv[3], NULL,
    gegl_node   ("gegl:over", NULL,
      gegl_node ("gegl:scale",
                 "x", 0.4,
                 "y", 0.4,
                 NULL,
      gegl_node ("gegl:invert",
                 NULL,
      gegl_node ("gegl:load",
                 "path", argv[1],
                 NULL
    ))),
      gegl_node ("gegl:translate",
                 "x", 50.0,
                 "y", 50.0,
                 NULL,
      gegl_node ("gegl:dropshadow",
                 "opacity", 1.0,
                 "radius", 3.0,
                 "x", 3.0,
                 "y", 3.0,
                 NULL,
      gegl_node ("gegl:text",
                 "size", 40.0,
                 "font", "sans bold",
                 "string", argv[2],
                 "color", gegl_color_new("green"),
                 NULL
    ))))));

  gegl_node_process (sink);
  g_object_unref (gegl);
  gegl_exit ();
  return 0;
}
