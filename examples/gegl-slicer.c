#include <stdlib.h>
#include <gegl.h>
#include <string.h>

gint
main (gint    argc,
      gchar **argv)
{
  gchar *rules;
  GeglNode *gegl,
           *load_file,
           *crop,
           *save_file;
  GError *error = NULL;

  gegl_init (&argc, &argv);

  if (argv[1]==NULL ||
      argv[2]==NULL)
    {
      g_print ("\nusage: %s <inputimage> <slicefile> [pathprefix]\n\n"
 "  inputimage: the .png, .jpg, .svg or similar image to load\n"
 "  slicefile: a text file with rule lines like (x,y widthxheight slice1.png)\n"
 "  pathprefix: a string to preprend to the names of slices in theslicefile\n", strrchr (argv[0], '/')+1);
      exit (-1);
    }

  gegl = gegl_node_new ();
  load_file = gegl_node_new_child (gegl,
                              "operation", "gegl:load",
                              "path", argv[1],
                              NULL);

  crop = gegl_node_new_child (gegl,
                              "operation", "gegl:crop",
                              NULL);

  save_file = gegl_node_new_child (gegl,
                                   "operation", "gegl:png-save",
                                   "compression", 9,
                                   "bitdepth", 8,
                                   NULL);
  gegl_node_link_many (load_file, crop, save_file, NULL);

  if (g_file_get_contents (argv[2], &rules, NULL, &error))
    {
      gchar *line;
      gchar *next_line = NULL;
      
      for (line = rules;line; line = next_line)
        {
          gchar *next_new_line;
          next_new_line = strchr (line, '\n');
          if (next_new_line)
            {
              next_line = next_new_line + 1;
              *next_new_line = '\0';
            }
          else
            {
              next_line = NULL;
            }

          while (*line == ' ')line++;

          if (line[0]!='#' && line[0]!='\0')
            {
              gint x, y, width, height;
              x = strtol (line, &line, 10);
              while (*line == ' ' || *line == ',')line++;
              y = strtol (line, &line, 10);
              while (*line == ' ' || *line == ',')line++;
              width = strtol (line, &line, 10);
              while (*line == ' ' || *line == 'x')line++;
              height = strtol (line, &line, 10);
              while (*line == ' ')line++;

              {
                GString *str = g_string_new ("");
                if (argv[3])
                  {
                    g_string_append (str, argv[3]);
                  }
                g_string_append (str, line);
                gegl_node_set (save_file, "path", str->str, NULL);
              gegl_node_set (crop, "x", x*1.0,
                                   "y", y*1.0,
                                   "width", width*1.0,
                                   "height", height * 1.0,
                                   NULL);
              g_print ("%s (%i,%i %ix%i)\n", str->str, x, y, width, height);
              g_string_free (str, TRUE);
            }
              gegl_node_process (save_file);
            }
        }
      g_free (rules);
    } 
  return 0;
}
