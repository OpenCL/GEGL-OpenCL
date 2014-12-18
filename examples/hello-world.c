#include <gegl.h>
#include <glib/gprintf.h>


gint
main (gint    argc,
      gchar **argv)
{
  gegl_init (&argc, &argv);  /* initialize the GEGL library */

  /* license for this application, needed by fractal-explorer */
  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);

  {
    /* instantiate a graph */
    GeglNode *gegl = gegl_node_new ();

/*
This is the graph we're going to construct:

.-----------.
| display   |
`-----------'
    |
.--------.
|  crop  |
`--------'
    |
.--------.
|  over  |
`--------'
    |   \
    |    \
    |     \
    |      |
    |   .------.
    |   | text |
    |   `------'
.------------------.
| fractal-explorer |
`------------------'

*/

    /*< The image nodes representing operations we want to perform */
    GeglNode *display    = gegl_node_create_child (gegl, "gegl:display");
    GeglNode *crop       = gegl_node_new_child (gegl,
                                 "operation", "gegl:crop",
                                 "width", 512.0,
                                 "height", 384.0,
                                  NULL);
    GeglNode *over       = gegl_node_new_child (gegl,
                                 "operation", "gegl:over",
                                 NULL);
    GeglNode *text       = gegl_node_new_child (gegl,
                                 "operation", "gegl:text",
                                 "size", 10.0,
                                 "color", gegl_color_new ("rgb(1.0,1.0,1.0)"),
                                 NULL);
    GeglNode *mandelbrot = gegl_node_new_child (gegl,
                                "operation", "gegl:fractal-explorer",
                                "shiftx", -256.0,
                                NULL);

    gegl_node_link_many (mandelbrot, over, crop, display, NULL);
    gegl_node_connect_to (text, "output",  over, "aux");

    /* request that the save node is processed, all dependencies will
     * be processed as well
     */
    {
      gint frame;
      gint frames = 200;

      for (frame=0; frame<frames; frame++)
        {
          gchar string[512];
          gdouble t = frame * 1.0/frames;

#define INTERPOLATE(min,max) ((max)*(t)+(min)*(1.0-t))

          gdouble shiftx = INTERPOLATE(-256.0, -512.0);
          gdouble shifty = INTERPOLATE(0.0,    -256.0);
          gdouble zoom   = INTERPOLATE(300.0,   400.0);

          gegl_node_set (mandelbrot, "shiftx", shiftx,
                                     "shifty", shifty,
                                     "zoom", zoom,
                                     NULL);
          g_sprintf (string, "x=%1.3f y=%1.3f z=%1.3f", shiftx, shifty, zoom);
          gegl_node_set (text, "string", string, NULL);
          gegl_node_process (display);
        }
    }

    /* free resources used by the graph and the nodes it owns */
    g_object_unref (gegl);
  }

  /* free resources globally used by GEGL */
  gegl_exit ();

  return 0;
}
