#include <gegl.h>
#include <glib/gprintf.h>

gint
main (gint    argc,
      gchar **argv)
{
  gegl_init (&argc, &argv);  /* initialize the GEGL library */

  {
    /* instantiate a graph */
    GeglNode *gegl = gegl_node_new ();

/*
This is the graph we're going to construct:
 
.-----------.
| ff-save   |
`-----------'
   |
.-------.
| layer |
`-------'
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
    GeglNode *display    = gegl_node_new_child (gegl,
                                 "operation", "gegl:ff-save",
                                 "path", "fractal-zoom.avi",
                                 "bitrate", 1200000.0,
                                 NULL);
    GeglNode *layer      = gegl_node_new_child (gegl,
                                 "operation", "gegl:layer",
                                 "x", 2.0,
                                 "y", 4.0,
                                 NULL);
    GeglNode *text       = gegl_node_new_child (gegl,
                                 "operation", "gegl:text",
                                 "size", 10.0,
                                 "color", gegl_color_new ("rgb(1.0,1.0,1.0)"),
                                 NULL);
    GeglNode *mandelbrot = gegl_node_new_child (gegl,
                                "operation", "gegl:fractal-explorer",
                                "width", 640,
                                "height", 480,
                                NULL);

    gegl_node_link_many (mandelbrot, layer, display, NULL);
    gegl_node_connect_to (text, "output",  layer, "aux");
   
    /* request that the save node is processed, all dependencies will
     * be processed as well
     */
    {
      gint frame;
      gint frames = 100;

      for (frame=0; frame<frames; frame++)
        {
          gchar string[512];
          gdouble t = frame * 1.0/frames;
          gdouble cx = -1.76;
          gdouble cy = 0.0;

#define INTERPOLATE(min,max) ((max)*(t)+(min)*(1.0-t))

          gdouble xmin = INTERPOLATE(  cx-0.02, cx-2.5);
          gdouble ymin = INTERPOLATE(  cy-0.02, cy-2.5);
          gdouble xmax = INTERPOLATE(  cx+0.02, cx+2.5);
          gdouble ymax = INTERPOLATE(  cy+0.02, cy+2.5);

          if (xmin<-3.0)
            xmin=-3.0;
          if (ymin<-3.0)
            ymin=-3.0;

          gegl_node_set (mandelbrot, "xmin", xmin,
                                     "ymin", ymin,
                                     "xmax", xmax,
                                     "ymax", ymax,
                                     NULL);
          g_sprintf (string, "%1.3f,%1.3f %1.3f×%1.3f",
            xmin, ymin, xmax-xmin, ymax-ymin);
          gegl_node_set (text, "string", string, NULL);
          gegl_node_process (display);
          g_print ("%3.0f%%  \r", t * 100);
        }
    }
    g_print ("      \r");

    /* free resources used by the graph and the nodes it owns */
    g_object_unref (gegl);
  }

  /* free resources globally used by GEGL */
  gegl_exit ();

  return 0;
}
