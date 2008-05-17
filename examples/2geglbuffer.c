#include <glib.h>
#include <gegl.h>

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer *buffer;
  GeglNode   *gegl, *load_file, *save_file;

  gtk_init (&argc, &argv);

  gegl_init (&argc, &argv);


  if (argv[1]==NULL ||
      argv[2]==NULL)
    {
      g_print ("\nusage: %s in.png out.gegl\n\nCreates a GeglBuffer from an image file.\n\n", argv[0]);
      exit (-1);
    }

  gegl = gegl_node_new ();
  load_file = gegl_node_new_child (gegl,
                              "operation", "load",
                              "path", argv[1],
                              NULL);
  save_file = gegl_node_new_child (gegl,
                                     "operation", "save-buffer",
                                     "buffer", &buffer,
                                     NULL);

  gegl_node_link_many (load_file, save_file, NULL);
  gegl_node_process (save_file);

 
  gegl_buffer_save (buffer, argv[2], NULL);
  return 0;
}
