#include "config.h"

#include <gegl-plugin.h>  /* needed to do full introspection */
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static GList *
gegl_operations_build (GList *list, GType type)
{
  GeglOperationClass *klass;
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return list;

  klass = g_type_class_ref (type);
  if (klass->name != NULL)
    list = g_list_prepend (list, klass);

  ops = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      list = gegl_operations_build (list, ops[no]);
    }
  if (ops)
    g_free (ops);
  return list;
}

static gint compare_operation_names (gconstpointer a,
                                     gconstpointer b)
{
  const GeglOperationClass *klassA, *klassB;

  klassA = a;
  klassB = b;

  return strcmp (klassA->name, klassB->name);
}

static GList *gegl_operations (void)
{
  static GList *operations = NULL;
  if (!operations)
    {
      operations = gegl_operations_build (NULL, GEGL_TYPE_OPERATION);
      operations = g_list_sort (operations, compare_operation_names);
    }
  return operations;
}

GeglColor *
gegl_param_spec_color_get_default (GParamSpec *self);


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
            /*G_OBJECT_CLASS (g_type_class_peek_parent (g_type_class_ref (type))),*/
            G_OBJECT_CLASS (g_type_class_ref (GEGL_TYPE_OPERATION)),
            &n_parent);

  for (prop_no=0;prop_no<n_self;prop_no++)
    {
      gint parent_no;
      gboolean found=FALSE;
      for (parent_no=0;parent_no<n_parent;parent_no++)
        if (self[prop_no]==parent[parent_no])
          found=TRUE;
      /* only print properties if we are an addition compared to
       * GeglOperation
       */
      if (!found)
        {
          const gchar *type_name = g_type_name (G_OBJECT_TYPE (self[prop_no]));

          type_name = strstr (type_name, "Param");
          type_name+=5;

          g_print("<tr><td colspan='1'>&nbsp;&nbsp;</td><td colspan='1' class='prop_type' valign='top'>%s<br/><span style='font-style:normal;text-align:right;float:right;padding-right:1em;'>", type_name);

          if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_DOUBLE))
            {

            g_print ("%2.2f", G_PARAM_SPEC_DOUBLE (self[prop_no])->default_value);

            {
            gdouble min = G_PARAM_SPEC_DOUBLE (self[prop_no])->minimum;
            gdouble max = G_PARAM_SPEC_DOUBLE (self[prop_no])->maximum;
            g_print ("<br/>");
            if (min<-10000000)
              g_print ("-inf ");
            else
              g_print ("%2.2f", min);
            
            g_print ("-");

            if (max>10000000)
              g_print (" +inf");
            else
              g_print ("%2.2f", max);
            }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_INT))
            {
              g_print ("%i", G_PARAM_SPEC_INT (self[prop_no])->default_value);

            {
            gint min = G_PARAM_SPEC_INT (self[prop_no])->minimum;
            gint max = G_PARAM_SPEC_INT (self[prop_no])->maximum;
            g_print ("<br/>");
            if (min<-10000000)
              g_print ("-inf ");
            else
              g_print ("%i", min);
            
            g_print ("-");

            if (max>10000000)
              g_print (" +inf");
            else
              g_print ("%i", max);
            }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_FLOAT))
            {
              g_print ("%2.2f", G_PARAM_SPEC_FLOAT (self[prop_no])->default_value);
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_BOOLEAN))
            {
              g_print ("%s", G_PARAM_SPEC_BOOLEAN (self[prop_no])->default_value?"True":"False");
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_STRING))
            {
              const gchar *string = G_PARAM_SPEC_STRING (self[prop_no])->default_value;

              if (strlen (string) > 8)
                {
                  gchar copy[16];
                  g_snprintf (copy, 12, "%s..", string);
                  g_print ("%s", copy);
                }
              else
                g_print ("%s", string);
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), GEGL_TYPE_COLOR))
            {
              GeglColor *color = gegl_param_spec_color_get_default (self[prop_no]);
              if (color)
                {
                  gchar *string;

                  g_object_get (color, "string", &string, NULL);
                  g_print ("%s", string);
                  g_free (string);

                  g_object_unref (color);
                }
            }
          else
            {
              g_print ("\n");
            }
          g_print ("</span></td>");

          g_print("<td class='prop_name' valign='top'>%s</td>\n",
            g_param_spec_get_name (self[prop_no]));

          if (g_param_spec_get_blurb (self[prop_no])[0]!='\0')
            g_print ("<td colspan='1' valign='top' class='prop_blurb'>%s</td>\n",
            g_param_spec_get_blurb (self[prop_no]));
          else
            g_print ("<td><em>not documented</em></td>\n\n");

          g_print ("</tr>\n");

        }
    }
  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
}


static gchar *html_top = "<html>\n<head>\n<title>GEGL operations</title>\n<link rel='shortcut icon' href='images/gegl.ico'/>\n<style type='text/css'>\n@import url(gegl.css);\ndiv#toc ul { font-size:70%; }\n"
".category { margin-bottom: 2em; }\n"
".category a {\n  display: block;\n  width: 10em;\n  height: 1.2em;\n  float: left;\n  text-align: left;\n  font-size: 90%;\n}\n"
"</style>\n</head>\n\n<body>\n<div class='paper'>\n<div class='content'>\n";
static gchar *html_bottom = "</div>\n</div>\n</body>\n</html>\n";


static void category_index (gpointer key,
                            gpointer value,
                            gpointer user_data)
{
  gchar    *category = key;
  GList    *operations = value;
  GList    *iter;
  gboolean  comma;

  if (!strcmp (category, "hidden"))
    return;
  g_print ("<a name='cat_%s'></a><h3>%s</h3>\n", category, category);
  g_print ("<div class='category'>\n");

  for (iter=operations, comma=FALSE;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      if (strstr (klass->categories, "hidden"))
        continue;
      g_print ("%s<a href='#op_%s'>%s</a>\n", comma?"":"", klass->name, klass->name);
      comma = TRUE;
    }
  g_print ("<div style='clear:both;'></div></div>\n");
}

static void category_menu_index (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
  gchar    *category = key;
  GList    *operations = value;
  GList    *iter;
  gboolean  comma;

  if (!strcmp (category, "hidden"))
    return;
  for (iter=operations, comma=FALSE;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      if (strstr (klass->categories, "hidden"))
        continue;
      g_print ("<li><a href='#op_%s'>%s</a></li>\n", klass->name, klass->name);
      comma = TRUE;
    }
}

gint
main (gint    argc,
      gchar **argv)
{
  GList      *operations;
  GList      *iter;
  GHashTable *categories = NULL;
  gboolean    comma;

  gegl_init (&argc, &argv);

  operations = gegl_operations ();

  /* Collect categories */
  categories = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  for (iter=operations, comma=FALSE;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      const gchar *ptr = klass->categories;
      while (*ptr)
        {
          gchar category[64]="";
          gint i=0;
          while (*ptr && *ptr!=':' && i<63)
            {
              category[i++]=*(ptr++);
              category[i]='\0';
            }
          if (*ptr==':')
            ptr++;
          {
            GList *items = g_hash_table_lookup (categories, category);
            g_hash_table_insert (categories, g_strdup (category), g_list_append (items, klass));
          }
        }
    }

  g_print ("%s", html_top);

  g_print ("<div id='toc'>\n<ul>\n");
  g_print ("<li><a href='index.html'>GEGL</a></li><li>&nbsp;</li>\n");
  g_print ("<li><a href='index.html#Documentation'>Documentation</a></li>\n");
  g_print ("<li><a href='index.html#Glossary'>&nbsp;&nbsp;Glossary</a></li>\n");
  g_print ("<li><a href='operations.html#'>&nbsp;&nbsp;Operations</a></li>\n");
  g_print ("<li><a href='api.html'>&nbsp;&nbsp;API reference</a></li>\n");
  g_print ("<li><a href=''>&nbsp;</a></li>\n");
  g_print ("<li><a href='#Categories'>Categories</a></li>\n");
  g_print ("<li><a href=''>&nbsp;</a></li>\n");
  /*category_menu_item ("All", NULL, NULL);
  g_hash_table_foreach (categories, category_menu_item, NULL);*/

      /*border: 0.1em dashed rgb(210,210,210);
       */
  category_menu_index("All", operations, NULL);

  g_print ("</ul>\n</div>\n");

    g_print ("<h1>GEGL operation reference</h1>");
    g_print ("<p>Image processing operations are shared objects (plug-ins) loaded when GEGL initializes. "
             "This page is generated from information registered by the plug-ins themselves.</p>"
              "<a name='Categories'><h2>Categories</h2></a><p>A plug-in can "
             "belong in multiple categories. Below is indexes broken down into the various available categories.</p>");

  /*category_index ("All", operations, NULL);*/
  /* create menus for each of the categories */

  g_hash_table_foreach (categories, category_index, NULL);

  /* list all operations */
  g_print ("<table>\n");
  for (iter=operations;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      if (strstr (klass->categories, "hidden"))
        continue;

      g_print ("<tr>\n  <td colspan='1'>&nbsp;</td>\n  <td class='op_name' colspan='4'><a name='op_%s'>%s</a></td>\n</tr>\n", klass->name, klass->name);
      if (klass->description)
        g_print ("<tr>\n  <td colspan='1'>&nbsp;</td>\n  <td class='op_description' colspan='4'>%s</td>\n</tr>\n", klass->description);
      list_properties (G_OBJECT_CLASS_TYPE (klass), 2, TRUE);
    }
  g_print ("</table>\n");


  g_print ("%s", html_bottom);

  g_list_free (operations);
  gegl_exit ();
  return 0;
}
