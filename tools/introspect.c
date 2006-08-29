#include <gegl.h>
#include <stdio.h>

FILE *file = NULL;

void collapse_all (GType type);
void expand_all (GType type);

static gchar *collapsibles_html =
"   <script type='text/javascript'>"
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
"    </script>";

void  introspect_overview (GType type, gint  indent);
void  introspect (GType type, gint  indent);
gint  stuff      (gint    argc, gchar **argv);

void introspection (void)
{
  file = stdout;
  fprintf (file, "<html><head><title>GObject class introspection</title><style type='text/css'>@import url(gegl.css);</style>%s</head><body>\n", collapsibles_html);
  fprintf (file, "<div class='toc'><ul><li><a href='index.html'>GEGL</a></li><li><a href='#top'>Class hierarchy</a></li>\n");
  fprintf (file, "</ul></div>\n");
  fprintf (file, "<div class='paper'><div class='content'>\n");


  fprintf (file, "<a name='#top'></a>\n");
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
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;
  gboolean first=TRUE;

  if (!type)
    return;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
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
              g_param_spec_get_blurb (self[prop_no]));
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
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;
  gboolean first=TRUE;

  if (!type)
    return;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
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
              g_param_spec_get_blurb (self[prop_no]));
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
              g_param_spec_get_blurb (self[prop_no]));
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

  /* exclude plug-ins */
  if(0){ 
    GTypeClass *class = NULL;
    class = g_type_class_ref (type);
    if (GEGL_IS_OPERATION_CLASS (class) &&
        GEGL_OPERATION_CLASS(class)->name!=NULL)
      {
        g_type_class_unref (class);
        return;
      }
    g_type_class_unref (class);
  }

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

  /* exclude plug-ins */
  if(0){ 
    GTypeClass *class = NULL;
    class = g_type_class_ref (type);
    if (GEGL_IS_OPERATION_CLASS (class) &&
        GEGL_OPERATION_CLASS(class)->name!=NULL)
      {
        g_type_class_unref (class);
        return;
      }
    g_type_class_unref (class);
  }

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

  /* exclude plug-ins */
  if(0){ 
    GTypeClass *class = NULL;
    class = g_type_class_ref (type);
    if (GEGL_IS_OPERATION_CLASS (class) &&
        GEGL_OPERATION_CLASS(class)->name!=NULL)
      {
        g_type_class_unref (class);
        return;
      }
    g_type_class_unref (class);
  }

  fprintf (file, "<hr/>\n");
  fprintf (file, "<a name='%s'><h3>%s</h3></a>\n", g_type_name (type),
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

  /* exclude plug-ins */
  if(0){
    GTypeClass *class = NULL;
    class = g_type_class_ref (type);
    if (GEGL_IS_OPERATION_CLASS (class) &&
        GEGL_OPERATION_CLASS(class)->name!=NULL)
      {
        g_type_class_unref (class);
        return;
      }
    g_type_class_unref (class);
  }

  /*fprintf (file, "<div>");
  for (i=0; i<indent; i++)
    fprintf (file, "&nbsp;&nbsp;&nbsp;");*/

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
  gegl_init (&argc, &argv);
  
    {
      GeglGraph *gegl = g_object_new (GEGL_TYPE_GRAPH, NULL);
      GeglNode  *display         = gegl_graph_create_node (gegl,
                    "class", "png-save",
                    "path",      "/tmp/ick",
                    NULL);
      GeglNode  *png_load      = gegl_graph_create_node (gegl,
                    "class", "png-load",
                    "path",      "data/100x100.png",
                    NULL);

      /* connect operations */
      gegl_node_connect   (display,     "input", png_load,   "output");

      /* then the whole output region */
      gegl_node_apply (display, "output");

      g_object_unref (gegl);
    }
  return 0;
}
