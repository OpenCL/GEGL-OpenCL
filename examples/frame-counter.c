#include <gegl.h>
#include <stdio.h>

const char *output_path = "frame-counter.ogv";

gint
main (gint    argc,
      gchar **argv)
{
  if (argv[1])
    output_path = argv[1];

  gegl_init (&argc, &argv);  /* initialize the GEGL library */
  {
    GeglNode *gegl  = gegl_node_new ();
    GeglNode *store = gegl_node_new_child (gegl,
                              "operation", "gegl:ff-save",
                              "path", output_path,
                              "frame-rate", 30.0,
                              NULL);
    GeglNode *crop    = gegl_node_new_child (gegl,
                              "operation", "gegl:crop",
                              "width", 512.0,
                              "height", 384.0,
                               NULL);
    GeglNode *over    = gegl_node_new_child (gegl,
                              "operation", "gegl:over",
                              NULL);
    GeglNode *text    = gegl_node_new_child (gegl,
                              "operation", "gegl:text",
                              "size", 120.0,
                              "color", gegl_color_new ("rgb(1.0,0.0,1.0)"),
                              NULL);
    GeglNode *bg      = gegl_node_new_child (gegl,
                             "operation", "gegl:color",
                              "value", gegl_color_new ("rgb(0.1,0.2,0.3)"),
                             NULL);

    gegl_node_link_many (bg, over, crop, store, NULL);
    gegl_node_connect_to (text, "output",  over, "aux");

    {
      gint frame;
      gint frames = 400;

      for (frame=0; frame < frames; frame++)
        {
          gchar string[512];
          gdouble t = frame * 1.0/frames;
#define INTERPOLATE(min,max) ((max)*(t)+(min)*(1.0-t))

          sprintf (string, "#%i\n%1.2f%%", frame, t*100);
          gegl_node_set (text, "string", string, NULL);
          fprintf (stderr, "\r%i% 1.2f%% ", frame, t*100);
          gegl_node_process (store);
        }
    }
    g_object_unref (gegl);
  }
  gegl_exit ();
  return 0;
}
