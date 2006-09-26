#include <gegl-plugin.h>  /* needed to do full introspection */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gegl-operation-filter.h"
#include "gegl-operation-source.h"
#include "gegl-operation-composer.h"

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

gint compare_operation_names (gconstpointer a,
                              gconstpointer b)
{
  const GeglOperationClass *klassA, *klassB;

  klassA = a;
  klassB = b;

  return strcmp (klassA->name, klassB->name);
}

GList *gegl_operations (void)
{
  static GList *operations = NULL;
  if (!operations)
    {
      operations = gegl_operations_build (NULL, GEGL_TYPE_OPERATION);
      operations = g_list_sort (operations, compare_operation_names);
    }
  return operations;
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
          g_print("<tr><td colspan='1'>&nbsp;&nbsp;</td><td colspan='1' class='prop_type' valign='top'>%s</td><td class='prop_name' valign='top'>%s</td>\n",
           type_name,
            g_param_spec_get_name (self[prop_no]));
          if (g_param_spec_get_blurb (self[prop_no])[0]!='\0')
            g_print ("<td colspan='1' class='prop_blurb'>%s</td></tr>\n",
            g_param_spec_get_blurb (self[prop_no]));
          else
            g_print ("<td><em>not documented</em></td></tr>");
        }
    }
  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
}


static gchar *html_top = "<html><head><title>GEGL operations</title><link rel='shortcut icon' href='images/gegl.ico'/><style type='text/css'>@import url(gegl.css);</style></head><body><div class='paper'><div class='content'>\n";
static gchar *html_bottom = "</div></div></body></html>";


void category_menu_item (gpointer key,
                         gpointer value,
                         gpointer user_data)
{
  gchar    *category = key;
  if (!strcmp (category, "hidden"))
    return;
  g_print ("<li><a href='#cat_%s'>&nbsp;&nbsp;%s</a></li>\n", category, category);
}

void category_index (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  gchar    *category = key;
  GList    *operations = value;
  GList    *iter;
  gboolean  comma;

  if (!strcmp (category, "hidden"))
    return;
  g_print ("<a name='cat_%s'><h3>%s</h3></a>\n", category, category);
  g_print ("<p>\n");

  for (iter=operations, comma=FALSE;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      if (strstr (klass->categories, "hidden"))
        continue;
      g_print ("%s<a href='#op_%s'>%s</a>", comma?", ":"", klass->name, klass->name);
      comma = TRUE;
    }
  g_print ("</p>\n");
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

  g_print (html_top);

  g_print ("<div class='toc'><ul>\n");
  g_print ("<li><a href='index.html'>GEGL</a></li><li>&nbsp;</li>\n");
  g_print ("<li><a href='operations.html#'>Operations</a></li>\n");
  category_menu_item ("All", NULL, NULL);
  g_hash_table_foreach (categories, category_menu_item, NULL);
  g_print ("</ul></div>\n");

    g_print ("<h1>Operations</h1>");
    g_print ("<p>Image processing operations are shared objects (plug-ins) loaded when GEGL initializes. "
             "This page is generated from information registered by the plug-ins themselves. A plug-in can "
             "belong in multiple categories.</p>");

  category_index ("All", operations, NULL);
  /* create menus for each of the categories */

  g_hash_table_foreach (categories, category_index, NULL);

  /* list all operations */
  g_print ("<table>");
  for (iter=operations;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      if (strstr (klass->categories, "hidden"))
        continue;

      g_print ("<tr><td colspan='1'>&nbsp;</td><td class='op_name' colspan='4'><a name='op_%s'>%s</a></td></tr>\n", klass->name, klass->name);
      if (klass->description)
        g_print ("<tr><td colspan='1'>&nbsp;</td><td class='op_description' colspan='4'>%s</td></tr>\n", klass->description);
      list_properties (G_OBJECT_CLASS_TYPE (klass), 2, TRUE);
    }
  g_print ("</table>");


  g_print (html_bottom);

  g_list_free (operations);
  gegl_exit ();
  return 0;
}
