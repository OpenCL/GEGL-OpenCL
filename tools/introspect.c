#include "config.h"
#include <gegl.h> /* needed instead of gegl.h to be able to do full
                     introspection*/
#include <stdio.h>

FILE *file = NULL;

void collapse_all (GType type);
void expand_all (GType type);

static gchar *collapsibles_html =
"   <script type='text/javascript'>/*<!--*/"
"    function hide(id)"
"    {"
"      (document.getElementById(id)).style.display = \"none\";"
"    }"
"    function show(id)"
"    {"
"      (document.getElementById(id)).style.display = \"block\";"
"    }"
"    function get_visible (id)"
"    {"
"      var element = document.getElementById(id);"
""
"      if (element &&"
"          element.style.display &&"
"          element.style.display != \"none\")"
"         return true;"
"      return false;"
"    }"
"    function set_visible (id, visible)"
"    {"
"      var element = document.getElementById(id);"
""
"      if (element)"
"        {"
"          if (visible)"
"              element.style.display = \"block\";"
"          else"
"              element.style.display = \"none\";"
"        }"
"    }"
"    function toggle_visible (id)"
"    {"
"      if (get_visible(id))"
"        set_visible(id, false);"
"      else"
"        set_visible(id,true);"
"    }"
"    /*-->*/</script>";

static gchar *escape (const gchar *string)
{
  gchar buf[4095]="";
  const gchar *p=string;
  gint i=0;

  while (*p)
    {
      switch (*p)
        {
          case '<':
            buf[i++]='&';
            buf[i++]='l';
            buf[i++]='t';
            buf[i++]=';';
            break;
          case '>':
            buf[i++]='&';
            buf[i++]='g';
            buf[i++]='t';
            buf[i++]=';';
            break;
          case '&':
            buf[i++]='&';
            buf[i++]='a';
            buf[i++]='m';
            buf[i++]='p';
            buf[i++]=';';
            break;
          default:
            buf[i++]=*p;
        }
      p++;
      buf[i]='\0';
    }
  return g_strdup (buf);
}

void  introspect_overview (GType type, gint  indent);
void  introspect (GType type, gint  indent);
gint  stuff      (gint    argc, gchar **argv);

static void introspection (void)
{
  file = stdout;
  fprintf (file, "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'\n'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>\n"
                 "<html xmlns='http://www.w3.org/1999/xhtml' lang='en' xml:lang='en'>");

  fprintf (file, "<head><title>GObject class introspection</title><link rel='shortcut icon' href='images/gegl.ico'/><style type='text/css'>@import url(devhelp.css);</style>%s</head><body>\n", collapsibles_html);
  fprintf (file, "<div class='toc'><ul><li><a href='index.html'>GEGL</a></li><li>&nbsp;</li><li><a href='#top'>Class hierarchy</a></li>\n");
  fprintf (file, "</ul></div>\n");
  fprintf (file, "<div class='paper'><div class='content'>\n");


  fprintf (file, "<a name='top'></a>\n");
  fprintf (file, "<div>\n");
  fprintf (file, "<a href='javascript:");
  collapse_all (G_TYPE_OBJECT);
  fprintf (file, "'>&nbsp;-&nbsp;</a>/<a href='javascript:");
  expand_all (G_TYPE_OBJECT);
  fprintf (file, "'>&nbsp;+&nbsp;</a>");
  introspect_overview (G_TYPE_OBJECT,0);
  fprintf (file, "</div>\n");
  introspect (G_TYPE_OBJECT,0);
  fprintf (file, "</div></div></body></html>\n");
}

gint
main (gint argc,
      gchar **argv)
{
  stuff (argc, argv);
  introspection ();

  return 0;
}

static void
list_subclasses (GType type)
{
  GType      *children;
  guint       count;
  gint        no;

  if (!type)
    return;

  children=g_type_children (type, &count);

  if (!children)
    return;

  if (count)
    {
    fprintf (file, "<h4>subclasses</h4>\n");
    fprintf (file, "<div>\n");

    for (no=0; no<count; no++)
      {
        fprintf (file, "<a href='#%s'>%s</a><br/>\n", g_type_name (children[no]),
                                                     g_type_name (children[no]));

      }
    fprintf (file, "</div>\n");
    }
  g_free (children);
}

static void
list_superclasses (GType type)
{
  GString *str = g_string_new ("");
  gboolean last=TRUE;

  while (type != 0)
    {
      if (!last)
        g_string_prepend (str, "/");
      g_string_prepend (str, "</a>");
      g_string_prepend (str, g_type_name (type));
      g_string_prepend (str, "'>");
      g_string_prepend (str, g_type_name (type));
      g_string_prepend (str, "<a href='#");
      type = g_type_parent (type);
      last = FALSE;
    }
  fprintf (file, "%s\n", str->str);
  g_string_free (str, TRUE);
}


static void
list_properties_simple (GType type)
{
  GParamSpec **self;
  GParamSpec **parent = NULL;
  guint n_self;
  guint n_parent=0;
  gint prop_no;
  gboolean first=TRUE;

  if (!type)
    return;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
  if (g_type_parent (type))
    parent = g_object_class_list_properties (
              G_OBJECT_CLASS (g_type_class_ref (g_type_parent (type))),
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

          if (first)
            {
               fprintf (file, "<dl>");
               first = FALSE;
            }
          fprintf (file, "<dt><b>%s</b> <em>%s</em></dt><dd>%s</dd>\n", 
              g_param_spec_get_name (self[prop_no]),
              g_type_name (G_OBJECT_TYPE(self[prop_no])),
              escape (g_param_spec_get_blurb (self[prop_no])));
        }
    }
  if (!first)
    fprintf (file, "</dl>\n");

  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
}

static void
list_properties (GType type,
                 gint  indent)
{
  GParamSpec **self;
  GParamSpec **parent = NULL;
  guint n_self;
  guint n_parent = 0;
  gint prop_no;
  gboolean first=TRUE;

  if (!type)
    return;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
  if (g_type_parent (type))
  parent = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (g_type_parent (type))),
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

          if (first)
            {
               fprintf (file, "<h5>Properties</h5><dl>");
               first = FALSE;
            }
          fprintf (file, "<dt><b>%s</b> <em>%s</em></dt><dd>%s</dd>\n", 
              g_param_spec_get_name (self[prop_no]),
              g_type_name (G_OBJECT_TYPE(self[prop_no])),
              escape(g_param_spec_get_blurb (self[prop_no])));
        }
    }
  if (!first)
    fprintf (file, "</dl>\n");

  first = TRUE;

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
      if (found)
        {
          if (first)
            {
               fprintf (file, "<h5>Inherited Properties</h5><dl>");
               first = FALSE;
            }
          fprintf (file, "<dt><b>%s</b> <em>%s</em></dt><dd>%s</dd>\n", 
              g_param_spec_get_name (self[prop_no]),
              g_type_name (G_OBJECT_TYPE(self[prop_no])),
              escape(g_param_spec_get_blurb (self[prop_no])));
        }
    }
  if (!first)
    fprintf (file, "</dl>\n");


  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
}

static void
details_for_type (GType type)
{
  fprintf (file, "<div class='Class'>\n");

  list_properties (type, 0);
  fprintf (file, "</div>\n");
}



void
collapse_all (GType type)
{
  GType      *children;
  guint       count;
  gint        no;

  if (!type)
    return;

  fprintf (file, "hide(\"x_%s\");", g_type_name(type));

  children=g_type_children (type, &count);

  if (!children)
    return;

  for (no=0; no<count; no++)
    {
      collapse_all (children[no]);
    }
  g_free (children);
}

void
expand_all (GType type)
{
  GType      *children;
  guint       count;
  gint        no;

  if (!type)
    return;

  fprintf (file, "show(\"x_%s\");", g_type_name(type));

  children=g_type_children (type, &count);

  if (!children)
    return;

  for (no=0; no<count; no++)
    {
      expand_all (children[no]);
    }
  g_free (children);
}

void
introspect (GType type,
            gint  indent)
{
  GType      *children;
  guint       count;
  gint        no;

  if (!type)
    return;

  fprintf (file, "<hr/>\n");
  fprintf (file, "<h3><a name='%s'>%s</a></h3>\n", g_type_name (type),
                                                   g_type_name (type));
  list_superclasses (type);
  details_for_type (type);
  list_subclasses (type);

  children=g_type_children (type, &count);

  if (!children)
    return;

  for (no=0; no<count; no++)
    {
      introspect (children[no], indent+2);
    }
  g_free (children);
}

void
introspect_overview (GType type,
                     gint  indent)
{
  GType      *children;
  guint       count;
  gint        no;

  if (!type)
    return;

  fprintf (file, "<div class='expander'>\n"
                   "<div class='expander_title'><a href='javascript:toggle_visible(\"x_%s\");'>%s</a></div>\n"
                   "<div class='expander_content' id='x_%s'>",
    g_type_name (type), 
    g_type_name (type), 
    g_type_name (type)
    );

  children=g_type_children (type, &count);

  list_properties_simple (type);
  
  if (!children)
    return;

  for (no=0; no<count; no++)
    {
      introspect_overview (children[no], indent+1);
    }

  fprintf (file, "</div></div>");
  g_free (children);
}


gint
stuff (gint    argc,
      gchar **argv)
{
  g_thread_init (NULL);
  gegl_init (&argc, &argv);
  
    {
      GeglNode  *gegl = g_object_new (GEGL_TYPE_NODE, NULL);

      GeglNode  *save = gegl_node_new_child (gegl,
                    "operation", "gegl:png-save",
                    "path", "/dev/null",
                    NULL);
      GeglNode *crop = gegl_node_new_child (gegl,
       "operation", "gegl:crop",
       "x", 0,
       "y", 0,
       "width", 50,
       "height", 50,
       NULL);
      GeglNode  *png_load = gegl_node_new_child (gegl,
                    "operation", "gegl:checkerboard",
                    NULL);

      /* connect operations */
      gegl_node_link_many (png_load, crop, save, NULL);

      /* then the whole output region */
      gegl_node_process (save);
      g_object_unref (gegl);
    }
  return 0;
}
