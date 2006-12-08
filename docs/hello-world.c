#include <gegl.h>

gint
main (gint    argc,
      gchar **argv)
{
  gegl_init (&argc, &argv);  /* initialize the GEGL library */

  {
    /* instantiate a graph */
    GeglNode *gegl = gegl_graph_new ();

/*
This is the graph we're going to construct:
 
.-----------.
| display   |
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
.-----------------.
| FractalExplorer |
`-----------------'

*/

    /*< The image nodes representing operations we want to perform */
    GeglNode *display    = gegl_graph_create_node (gegl, "display");
    GeglNode *layer      = gegl_graph_new_node (gegl,
                                 "operation", "layer",
                                 "x", 2.0,
                                 "y", 4.0,
                                 NULL);
    GeglNode *text       = gegl_graph_new_node (gegl,
                                 "operation", "text",
                                 "size", 10.0,
                                 "color", gegl_color_new ("rgb(1.0,1.0,1.0)"),
                                 NULL);
    GeglNode *mandelbrot = gegl_graph_new_node (gegl,
                                "operation", "FractalExplorer",
                                "width", 256,
                                "height", 256,
                                NULL);

    gegl_node_link_many (mandelbrot, layer, display, NULL);
    gegl_node_connect_to (text, "output",  layer, "aux");
   
    /* request that the save node is processed, all dependencies will
     * be processed as well
     */
    {
      gint frame;
      gint frames = 30;

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
        }
    }

    /* free resources used by the graph and the nodes it owns */
    g_object_unref (gegl);
  }

  /* free resources globally used by GEGL */
  gegl_exit ();

  return 0;
}
