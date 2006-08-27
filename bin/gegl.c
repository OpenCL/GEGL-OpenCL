#include <gegl.h>
#include <gegl/gegl-xml.h> /* needed to build in source dir, but exposed
                              through gegl.h in other places */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gegl-options.h"
#include "gegl-operation-filter.h"
#include "gegl-operation-source.h"
#include "gegl-operation-composer.h"

static gchar *usage = "Usage: %s <xmlfile>\n"
"\n"
"Evaluates the supplied XML encoded GEGL processing graph.\n"
"If xmlfile is - the xml is read from standard input\n";

static void gegl_list_ops    (gboolean html);
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

  switch (o->mode)
    {
      case GEGL_RUN_MODE_OP_LISTING:
        gegl_list_ops (FALSE);
        return 0;
        break;
      case GEGL_RUN_MODE_OP_LISTING_HTML:
        gegl_list_ops (TRUE);
        return 0;
        break;
      default:
        break;
    }

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
      script = g_strdup ("<gegl><tree><node class='text' size='100' string='GEGL'/></tree></gegl>");
    }
  
  gegl = gegl_xml_parse (script);

  switch (o->mode)
    {
      case GEGL_RUN_MODE_INTERACTIVE:
        {
          main_interactive (gegl, o);
        }
        break;
      case GEGL_RUN_MODE_PNG:
        {
          GeglNode *output = gegl_graph_create_node (GEGL_GRAPH (gegl),
                               "class", "png-save",
                               "path", o->output,
                               NULL);
          gegl_node_connect (output, "input", gegl_graph_output (GEGL_GRAPH (gegl), "output"), "output");

          gegl_node_apply (output, "output");
        }
        break;
      case GEGL_RUN_MODE_HELP:
        break;
      default:
        break;
    }

  g_object_unref (gegl);

  g_free (o);
  gegl_exit ();
  return 0;
}

static gint 
main_interactive (GeglNode *gegl,
                  GeglOptions *o)
{
  GeglNode *output = gegl_graph_create_node (GEGL_GRAPH (gegl),
                       "class", "display",
                       NULL);

  gegl_node_connect (output, "input", gegl_graph_output (GEGL_GRAPH (gegl), "output"), "output");
  gegl_node_apply (output, "output");
  sleep(o->delay);
  return 0;
}



static void
list_properties (GType    type,
                 gint     indent,
                 gboolean html)
{
  GParamSpec **self;
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;

  if (!type)
    return;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
  parent = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_peek_parent (g_type_class_ref (type))),
            /*G_OBJECT_CLASS (g_type_class_ref (GEGL_TYPE_OPERATION)),*/
            &n_parent);

  for (prop_no=0;prop_no<n_self;prop_no++)
    {
      gint parent_no;
      gboolean found=FALSE;
      for (parent_no=0;parent_no<n_parent;parent_no++)
        if (self[prop_no]==parent[parent_no])
          found=TRUE;
      /* only print properties if we are an addition compared to
       * the parent class
       */
      if (!found)
        {
          gint i;
          for (i=0;i<indent;i++)
            g_print (" ");
          if (html)
            {
              g_print("<tr><td colspan='2'>&nbsp;&nbsp;</td><td colspan='1' class='prop_type'>%s</td><td class='prop_name'>%s</td></tr>\n",
                g_type_name (G_OBJECT_TYPE(self[prop_no])),
                g_param_spec_get_name (self[prop_no]));
              if (g_param_spec_get_blurb (self[prop_no])[0]!='\0')
                g_print ("<tr><td colspan='3'>&nbsp;</td><td colspan='2' class='prop_blurb'>%s</td></tr>\n",
                g_param_spec_get_blurb (self[prop_no]));
            }
          else
            {
              g_print("%s %s %s\n",
                g_type_name (G_OBJECT_TYPE(self[prop_no])),
                g_param_spec_get_name (self[prop_no]),
                g_param_spec_get_blurb (self[prop_no]));
            }
        }
    }
  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
}

static void
introspect (GType    type,
            gint     indent,
            gboolean html)
{
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return;

  ops = g_type_children (type, &children);

  if (!ops)
    return;

  for (no=0; no<children; no++)
    {
      GeglOperationClass *klass;
      gint i;
      g_print ("\n");
      for (i=0; i<indent; i++)
        g_print (" ");

      klass = g_type_class_ref (ops[no]);
      if (klass->name != NULL)
        {
          if (html)
            g_print ("<tr><td colspan='2'>&nbsp;</td><td class='op_name' colspan='3'><a name='%s'>%s</a></td></tr>\n", klass->name, klass->name);
          else
            g_print ("%s\n", klass->name);
          if (html && klass->description)
            g_print ("<tr><td colspan='2'>&nbsp;</td><td class='op_description' colspan='3'>%s</td></tr>\n", klass->description);
          list_properties (ops[no], indent+2, html);
        }
      introspect (ops[no], indent+2, html);
    }
  g_free (ops);
}

static void
index_introspect (GType    type)
{
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return;

  ops = g_type_children (type, &children);

  if (!ops)
    return;

  for (no=0; no<children; no++)
    {
      GeglOperationClass *klass;

      klass = g_type_class_ref (ops[no]);
      if (klass->name != NULL)
        g_print ("<li><a href='#%s'>%s</a></li>\n", klass->name, klass->name);
      index_introspect (ops[no]);
    }
  g_free (ops);
}

static void
gegl_list_ops (gboolean html)
{
  if (html)
    {
      g_print ("<html><head><title>GEGL operations</title><style type='text/css'>@import url(gegl.css);</style></head><body>\n");
      g_print ("<div class='toc'><ul><li><a href='index.html'>GEGL</a></li><li><a href='#'>Operations</a></li>\n");
      g_print ("<li><a href='#sources'>&nbsp;&nbsp;Sources</a></li>\n");
      g_print ("<li><a href='#filters'>&nbsp;&nbsp;Filters</a></li>\n");
      g_print ("<li><a href='#composers'>&nbsp;&nbsp;Composers</a></li>\n");
      g_print ("</ul></div>\n");
      g_print ("<div class='paper'><div class='content'>\n");
      g_print ("<h1>Operations</h1>");
      g_print ("<p>Image processing operations are loaded when GEGL initializes. In this listing generated from the metadata provided by operations themselves, they are divided into main categories, which also corresponds to different base classes.</p>");
    }

  if(html)
    g_print ("<table border='0'>");

  if(html)
    g_print ("<tr><td colspan='5'><a name='sources'></a><h2>Sources</h2>"
     "<p>Source operations have one output pad, but no input pads. Typical sources are file loaders, video loaders (providing a frame property), video sources (v4l, DV25), gradients, noise, text renderer ..</p><ul>");
      index_introspect (GEGL_TYPE_OPERATION_SOURCE);
     g_print ("</ul></td></tr>\n");
  introspect (GEGL_TYPE_OPERATION_SOURCE, 0, html);
  if(html)
    g_print ("<tr><td colspan='5'><a name='filters'></a><h2>Filters</h2>"
     "<p>Filter operations have an input pad and an output pad and change the image in some manner. Samples: blurs, color correction, sharpening, artistic filters ...</p><ul>");
      index_introspect (GEGL_TYPE_OPERATION_FILTER);
     g_print ("</ul></td></tr>\n");
  introspect (GEGL_TYPE_OPERATION_FILTER, 0, html);
  if(html)
    g_print ("<tr><td colspan='5'><a name='composers'></a><h2>Composers</h2>"
     "<p>Composer operations are like filters, but also have an additional auxiliary image buffer input pad.</p><ul>");
      index_introspect (GEGL_TYPE_OPERATION_COMPOSER);
     g_print ("</ul></td></tr>\n");
  introspect (GEGL_TYPE_OPERATION_COMPOSER, 0, html);

  if (html)
    g_print ("</table></div></div></body></html>");
}
