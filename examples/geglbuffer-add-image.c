#include <gegl.h>
#include <glib/gprintf.h>
#include <stdlib.h>

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer; /* instantiate a graph */
  GeglNode   *gegl;   /* the gegl graph we're using as a node factor */
  GeglNode   *write_buffer,
             *shift,
             *load;
  gchar *in_file;
  gchar *buf_file;
  gdouble x;
  gdouble y;

  gegl_init (&argc, &argv);  /* initialize the GEGL library */


  if (argv[1]==NULL ||
      argv[2]==NULL ||
      argv[3]==NULL ||
      argv[4]==NULL)
    {
      g_print ("\nUsage: %s <gegl buffer> <image file> <x> <y>\n"
               "\nWrites an image into the GeglBuffer at the specified coordinates\n",
               argv[0]);
      return -1;
    }
  buf_file = argv[1];
  in_file = argv[2];
  x = atof (argv[3]);
  y = atof (argv[4]);

  buffer = gegl_buffer_open (buf_file);
  gegl = gegl_node_new ();

  write_buffer = gegl_node_new_child (gegl,
                                    "operation", "gegl:write-buffer",
                                    "buffer", buffer,
                                    NULL);
  shift      = gegl_node_new_child (gegl,
                                    "operation", "gegl:translate",
                                    "x", x,
                                    "y", y,
                                    NULL);
  load        = gegl_node_new_child (gegl,
                                   "operation", "gegl:load",
                                   "path", in_file,
                                   NULL);

  gegl_node_link_many (load, shift, write_buffer, NULL);
  gegl_node_process (write_buffer);

  /* free resources used by the graph and the nodes it owns */
  g_object_unref (gegl);
  g_object_unref (buffer);

  /* free resources globally used by GEGL */
  gegl_exit ();

  return 0;
}
