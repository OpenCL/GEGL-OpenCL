#include <gegl.h>
#include <gegl/gegl-xml.h> /* needed to build in source dir, but exposed
                              through gegl.h in other places */
#include <stdio.h>
#include <string.h>
#include "gegl-instrument.h"
#include <unistd.h>
#include "gegl-options.h"

static gchar *usage = "Usage: %s <xmlfile>\n"
"\n"
"Evaluates the supplied XML encoded GEGL processing graph.\n"
"If xmlfile is - the xml is read from standard input\n";

static gint main_interactive (GeglNode    *gegl,
                              GeglOptions *o);

gint
main (gint    argc,
      gchar **argv)
{
  GeglOptions *o        = NULL;
  GeglNode    *gegl     = NULL;
  const gchar *filename = NULL;
  gchar       *script   = NULL;
  GError      *err      = NULL;

  if (argc <= 1)
    {
      fprintf (stderr, usage, argv[0]);
      exit (-1);
    }

  filename = argv[1];

  gegl_init (&argc, &argv);

  o = gegl_options_parse (argc, argv);

  if (o->xml)
    {
      script = g_strdup (o->xml);
    }
  else if (o->file)
    {
      if (!strcmp (o->file, "-"))
        {
          gint  buf_size = 128;
          gchar buf[buf_size];
          GString *acc = g_string_new ("");

          while (fgets (buf, buf_size, stdin))
            {
              g_string_append (acc, buf);
            }
          script = g_string_free (acc, FALSE);
        }
      else
        {
          g_file_get_contents (o->file, &script, NULL, &err);
          if (err != NULL)
            {
              g_warning ("Unable to read file: %s", err->message);
            }
        }
    }
  else
    {
      script = g_strdup ("<gegl><tree><node operation='text' size='100' string='GEGL'/></tree></gegl>");
    }

  gegl = gegl_xml_parse (script);

  switch (o->mode)
    {
      case GEGL_RUN_MODE_INTERACTIVE:
          main_interactive (gegl, o);
          g_object_unref (gegl);
          g_free (o);
          gegl_exit ();
          return 0;
        break;
      case GEGL_RUN_MODE_XML:
          g_print (gegl_to_xml (gegl));
          gegl_exit ();
          return 0;
        break;
      case GEGL_RUN_MODE_PNG:
        {
          glong timing;

          timing = gegl_ticks ();
          GeglNode *output = gegl_graph_create_node (GEGL_GRAPH (gegl),
                               "operation", "png-save",
                               "path", o->output,
                               NULL);
          if (o->stats)
            g_print ("%s  ", o->file);
          gegl_node_connect (output, "input", gegl_graph_output (GEGL_GRAPH (gegl), "output"), "output");

          gegl_node_apply (output, "output");
          timing = gegl_ticks()-timing;
          if (o->stats)
            g_print ("usecs: %li", timing);


          g_object_unref (gegl);
          g_free (o);
          gegl_exit ();

          if (gegl_buffer_leaks())
            g_print ("  buffer-leaks: %i", gegl_buffer_leaks ());
          g_print ("\n");
        }
        break;
      case GEGL_RUN_MODE_HELP:
        break;
      default:
        break;
    }

  return 0;
}

static gint
main_interactive (GeglNode *gegl,
                  GeglOptions *o)
{
  GeglNode *output = gegl_graph_create_node (GEGL_GRAPH (gegl),
                       "operation", "display",
                       NULL);

  gegl_node_connect (output, "input", gegl_graph_output (GEGL_GRAPH (gegl), "output"), "output");
  gegl_node_apply (output, "output");
  sleep(o->delay);
  return 0;
}
