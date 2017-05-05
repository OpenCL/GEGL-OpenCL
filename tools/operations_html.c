#include "config.h"
#include <gegl-plugin.h>

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

/* convert operation name to path of example image */
static gchar*
operation_to_image_path (const gchar *op_name)
{
  gchar *cleaned = g_strdup (op_name);
  gchar *filename, *output_path;

  g_strdelimit (cleaned, ":", '-');
  filename = g_strconcat (cleaned, ".png", NULL);
  output_path = g_build_path (G_DIR_SEPARATOR_S, "images", filename, NULL);

  g_free (cleaned);
  g_free (filename);

  return output_path;
}

static void json_escape_string (GString *s, const char *description)
{
  const char *p;
  if (!description)
    return;
  for (p = description; *p; p++)
  {
    switch (*p)
    {
      case '"':
      case '\\':
      case '/':
        g_string_append_printf (s, "\\%c", *p);
        break;
      case '\n':
        g_string_append_printf (s, "\\n");
        break;
      default:
        g_string_append_printf (s, "%c", *p);
    }
  }
}


static void xml_escape_string (GString *s, const char *description)
{
  const char *p;
  if (!description)
    return;
  for (p = description; *p; p++)
  {
    switch (*p)
    {
      case '<':
        g_string_append_printf (s, "&lt;");
        break;
      case '>':
        g_string_append_printf (s, "&gt;");
        break;
      case '&':
        g_string_append_printf (s, "&amp;");
        break;
      default:
        g_string_append_printf (s, "%c", *p);
    }
  }
}

static void
json_list_pads (GType type, GString *s, const gchar *opname)
{
  GeglNode *gegl = gegl_node_new ();
  GeglNode *node = gegl_node_new_child (gegl,
                            "operation", opname,
                            NULL);
  gchar **input_pads;
  gchar **output_pads;

  int i;

  input_pads = gegl_node_list_input_pads (node);
  output_pads = gegl_node_list_output_pads (node);

  g_string_append_printf (s, "<b>pads:</b>\n");
  if (input_pads && input_pads[0])
  {
    for (i = 0; input_pads[i]; i++)
    {
      g_string_append_printf (s, "%s\n", input_pads[i]);
    }
    g_free (input_pads);
  }

  if (output_pads && output_pads[0])
  {
    for (i = 0; output_pads[i]; i++)
    {
      g_string_append_printf (s, "%s\n", output_pads[i]);
    }
    g_free (output_pads);
  }
    g_string_append_printf (s, "<br/>");

  g_object_unref (gegl);
}

static void
json_list_properties (GType type, GString *s, const gchar *opname)
{
  GParamSpec **self;
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;

  if (!type)
    return;

  g_string_append_printf (s, "<div class='properties'>\n");

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
          const gchar *type_name;
          g_string_append_printf (s, "<div style='margin-top:0.5em;'><div style='padding-left:0.1em;border-left:0.3em solid black;margin-left:-0.35em;'>\n");

            json_escape_string (s, g_param_spec_get_nick (self[prop_no]));
          g_string_append_printf (s, "</div>\n");


          if (g_param_spec_get_blurb (self[prop_no]) &&
              g_param_spec_get_blurb (self[prop_no])[0]!='\0')
          {
            g_string_append_printf (s, "<div>");
            json_escape_string (s, g_param_spec_get_blurb (self[prop_no]));
            g_string_append_printf (s, "</div>\n");
          }

          g_string_append_printf (s, "<div style='font-size:70%%;'><b>name:&nbsp;</b>%s\n<b>type:&nbsp;</b>", g_param_spec_get_name (self[prop_no]));

          type_name = g_type_name (G_OBJECT_TYPE (self[prop_no]));
          if(strstr (type_name, "Param"))
          {
            type_name = strstr (type_name, "Param");
            type_name+=5;
          }
          {
            for (const char *p = type_name; *p; p++)
              g_string_append_printf (s, "%c", g_ascii_tolower (*p));
          }

          g_string_append_printf (s, "\n");

#if 1
          if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_DOUBLE))
            {
              gdouble default_value = G_PARAM_SPEC_DOUBLE (self[prop_no])->default_value;
              gdouble min = G_PARAM_SPEC_DOUBLE (self[prop_no])->minimum;
              gdouble max = G_PARAM_SPEC_DOUBLE (self[prop_no])->maximum;

              if (default_value<-10000000)
                g_string_append_printf (s, "<b>default:</b>&nbsp;-inf \n");
              else if (default_value>10000000)
                g_string_append_printf (s, "<b>default:</b>&nbsp;+inf \n");
              else
                g_string_append_printf (s, "<b>default:</b>&nbsp;%2.2f \n", default_value);

              if (min<-10000000)
                g_string_append_printf (s, "<b>minimum:</b>&nbsp;-inf \n");
              else
                g_string_append_printf (s, "<b>minimum:</b>&nbsp;%2.2f \n", min);

              if (max>10000000)
                g_string_append_printf (s, "<b>maximum:</b>&nbsp;+inf \n");
              else
                g_string_append_printf (s, "<b>maximum:</b>&nbsp;%2.2f \n", max);
              if (GEGL_IS_PARAM_SPEC_DOUBLE (self[prop_no]))
              {
                GeglParamSpecDouble *pspec =
                              GEGL_PARAM_SPEC_DOUBLE (self[prop_no]);

                if (pspec->ui_minimum < -10000000)
                  g_string_append_printf (s, "<b>ui-minimum:</b>&nbsp;-inf \n");
                else
                  g_string_append_printf (s, "<b>ui-minimum:</b>&nbsp;%2.2f \n", pspec->ui_minimum);

                if (pspec->ui_maximum > 10000000)
                  g_string_append_printf (s, "<b>ui-maximum:</b>&nbsp;+inf \n");
                else
                  g_string_append_printf (s, "<b>ui-maximum:</b>&nbsp;%2.2f \n", pspec->ui_maximum);

                g_string_append_printf (s, "<b>ui-gamma:</b>&nbsp;%2.2f \n", pspec->ui_gamma);
                g_string_append_printf (s, "<b>ui-step-small:</b>&nbsp;%2.2f \n", pspec->ui_step_small);
                g_string_append_printf (s, "<b>ui-step-big:</b>&nbsp;%2.2f \n", pspec->ui_step_big);
                g_string_append_printf (s, "<b>ui-digits:</b>&nbsp;%i \n", pspec->ui_digits);
              }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_INT))
            {
              gint default_value = G_PARAM_SPEC_INT (self[prop_no])->default_value;
              gint min = G_PARAM_SPEC_INT (self[prop_no])->minimum;
              gint max = G_PARAM_SPEC_INT (self[prop_no])->maximum;

              if (default_value<-10000000)
                g_string_append_printf (s, "<b>default:</b>&nbsp;-inf \n");
              else if (default_value>10000000)
                g_string_append_printf (s, "<b>default:</b>&nbsp;+inf \n");
              else
                g_string_append_printf (s, "<b>default:</b>&nbsp;%i \n", default_value);

              if (min<-10000000)
                g_string_append_printf (s, "<b>minimum:</b>&nbsp;-inf \n");
              else
                g_string_append_printf (s, "<b>minimum:</b>&nbsp;%i \n", min);

              if (max>10000000)
                g_string_append_printf (s, "<b>maximum:</b>&nbsp;+inf \n");
              else
                g_string_append_printf (s, "<b>maximum:</b>&nbsp;%i \n", max);

              if (GEGL_IS_PARAM_SPEC_INT (self[prop_no]))
              {
                GeglParamSpecInt *pspec =
                              GEGL_PARAM_SPEC_INT (self[prop_no]);

                if (pspec->ui_minimum < -10000000)
                  g_string_append_printf (s, "<b>ui-minimum:</b>&nbsp;-inf \n");
                else
                  g_string_append_printf (s, "<b>ui-minimum:</b>&nbsp;%i \n", pspec->ui_minimum);

                if (pspec->ui_maximum > 10000000)
                  g_string_append_printf (s, "<b>ui-maximum:</b>&nbsp;+inf \n");
                else
                  g_string_append_printf (s, "<b>ui-maximum:</b>&nbsp;%i \n", pspec->ui_maximum);

                g_string_append_printf (s, "<b>ui-gamma:</b>&nbsp;%2.2f \n", pspec->ui_gamma);
                g_string_append_printf (s, "<b>ui-step-small:</b>&nbsp;%i \n", pspec->ui_step_small);
                g_string_append_printf (s, "<b>ui-step-big:</b>&nbsp;%i \n", pspec->ui_step_big);
              }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_BOOLEAN))
            {
              g_string_append_printf (s, " <b>default:</b>&nbsp;%s \n", G_PARAM_SPEC_BOOLEAN (self[prop_no])->default_value?"True":"False");
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_STRING))
            {
              const gchar *string = G_PARAM_SPEC_STRING (self[prop_no])->default_value;

              g_string_append_printf (s, "  <b>default:</b>&nbsp;");
              json_escape_string (s, string);
              g_string_append_printf (s, " \n");

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), GEGL_TYPE_COLOR))
            {
              GeglColor *color = gegl_param_spec_color_get_default (self[prop_no]);
              if (color)
                {
                  gchar *string;

                  g_object_get (color, "string", &string, NULL);
                  g_string_append_printf (s, "<b>default:</b>&nbsp;");
                  json_escape_string (s, string);
                  g_string_append_printf (s, " \n");
                  g_free (string);
                }
            }
          else
            {
            }
#endif


      {
        guint count;
        gchar **property_keys = gegl_operation_list_property_keys (
            opname,
            g_param_spec_get_name (self[prop_no]),
            &count);

        if (property_keys)
        {

          int i;
          if (property_keys[0])
          {
            /* XXX: list is in reverse order */
            for (i = 0; property_keys[i]; i++)
            {
              g_string_append_printf (s, "<b>%s</b>:%s\n",
                    property_keys[i],
                    gegl_operation_get_property_key (opname,
                                      g_param_spec_get_name (self[prop_no]),
                                                property_keys[i]));
            }
          }
          g_free (property_keys);
        }

      }

          g_string_append_printf (s, "</div>");
          g_string_append_printf (s, "</div>");
        }
    }
  if (self)
    g_free (self);
  if (parent)
    g_free (parent);

  g_string_append_printf (s, "</div>");
}

GHashTable *seen_categories = NULL;

const gchar *css = "@import url(../gegl.css); .categories{ clear:right;text-align: justify; line-height: 1.6em; float:right; padding-left: 1em; padding-bottom: 1em ;width: 14em; } .categories a, .category {color:black; background:#eef; padding: 0.1em; padding-left0.2em; padding-right: 0.2em; border: 1px solid gray;} .description{margin-bottom:1em;}";

const gchar *html_pre = "<html><head><title>%s</title>\n"
                          "<style>%s%s</style></head><body><div id='content'>\n";
const gchar *html_post =
            "<div style='margin-top:3em;'><a href='../index.html'><img src='../images/GEGL.png' alt='GEGL' style='height: 4.0em;float:left; padding-right:0.5em;'/></a> This page is part of the online GEGL Documentation, GEGL is a data flow based image processing library/framework, made to fuel <a href='https://www.gimp.org/'>GIMPs</a> high-bit depth non-destructive editing future.</div></body></html>\n";

gint
main (gint argc, gchar **argv)
{
  GList      *operations;
  GList      *iter;
  GString    *s = g_string_new ("");

  gegl_init (&argc, &argv);
  seen_categories = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);

  operations = gegl_operations ();

  for (iter=operations;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;

      const char *name = gegl_operation_class_get_key (klass, "name");
      const char *title = gegl_operation_class_get_key (klass, "title");
      const char *categoris = gegl_operation_class_get_key (klass, "categories");
      const char *description = gegl_operation_class_get_key (klass, "description");

      g_string_append_printf (s, html_pre, name, css, ";body { margin: 1em 5% 1em 5%; }");
      g_string_append_printf (s, "<div class='operation'><h2 style='clear:both;'>%s</h2>\n", title?title:name);


      {
        char *image = operation_to_image_path (name);

        if (g_file_test (image, G_FILE_TEST_EXISTS))
          g_string_append_printf (s, "<img style='clear: both; padding-left: 1em; float: right; ' src='%s' />\n", image);
        g_free (image);
      }

      if (description)
      {
        g_string_append_printf (s, "<div class='description'>");
        xml_escape_string (s, description);
        g_string_append_printf (s, "</div>\n");
      }


      g_string_append_printf (s, "<div style='margin-bottom:1em;'>");
      json_list_properties (G_OBJECT_CLASS_TYPE (klass), s, name);
      g_string_append_printf (s, "</div>");

      g_string_append_printf (s, "<b>name:</b>&nbsp%s<br/>", name);
      json_list_pads (G_OBJECT_CLASS_TYPE (klass), s, name);

      g_string_append_printf (s, "<b>parent-class:</b>&nbsp;<a href='%s.html'>%s</a><br/>\n",
        g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass))),
        g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass)))
        );
      g_hash_table_insert (seen_categories, g_strdup (
        g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass)))
                              ), (void*)0xff);

      if (categoris)
      {
        const gchar *ptr = categoris;
        g_string_append_printf (s, "<b>categories:</b> ");

        while (ptr && *ptr)
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
              g_string_append_printf (s, "<a class='category' style='color:black' href='%s.html'>%s</a> ", category, category);
              g_hash_table_insert (seen_categories, g_strdup (category), (void*)0xff);
            }
          }
        g_string_append_printf (s, "<br/>\n");
      }

      if (klass->opencl_support)
        g_string_append_printf (s, "<b>OpenCL</b><br/>\n");

      if (klass->compat_name)
        g_string_append_printf (s, "<b>compat-op:</b>&nbsp;%s<br/>\n", klass->compat_name);

      if(1){ // XXX: re-enable before push.. it takes a lot of time
        gchar *commandline = g_strdup_printf (
            "sh -c \"(cd " TOP_SRCDIR ";cd ..;grep -r '\\\"%s\\\"' operations) | grep operations | grep -v '~:' | grep '\\\"name\\\"' | cut -f 1 -d ':'\"",
             name);
        gchar *output = NULL;
        
        if (g_spawn_command_line_sync (commandline, &output, NULL, NULL, NULL))
        {
          if (strlen(output))
            {
              output[strlen(output)-1] = 0;
              g_string_append_printf (s, 
      "<b>source:</b>&nbsp;<a href='https://git.gnome.org/browse/gegl/tree/%s'>%s</a><br/>\n", output, output);
            }
          g_free (output);
        }

        g_free (commandline);
      }

      {
        guint nkeys;
        gchar **keys = gegl_operation_list_keys (name, &nkeys);

        if (keys)
          {
            for (gint i = 0; keys[i]; i++)
              {
                const gchar *value = gegl_operation_get_key (name, keys[i]);

                if (g_str_equal (keys[i], "categories") ||
                    g_str_equal (keys[i], "cl-source") ||
                    g_str_equal (keys[i], "name") ||
                    g_str_equal (keys[i], "source") ||
                    g_str_equal (keys[i], "reference-composition") ||
                    g_str_equal (keys[i], "reference-hash") ||
                    g_str_equal (keys[i], "title") ||
                    g_str_equal (keys[i], "description"))
                  continue;

                g_string_append_printf (s, "<b>%s:</b>&nbsp;", keys[i]);
                json_escape_string (s, value);
                g_string_append_printf (s, "<br/>\n");
              }
            g_free (keys);
          }
      }

      g_string_append_printf (s, "</dl>");

      g_string_append_printf (s, "</div>");
      g_string_append (s, html_post);
      {
        gchar *html_name = g_strdup_printf ("%s.html", name);
        gchar *colon = strchr (html_name, ':');
        if (colon) *colon = '-';
        g_print ("%s\n", html_name);
        g_file_set_contents (html_name, s->str, -1, NULL);
        g_string_assign (s, "");
        g_free (html_name);
      }
    }

   if(1){
      gchar *category = "all";
      GList *keys = g_hash_table_get_keys (seen_categories);
      GString *cs;
      GList *k = keys;
      GList *keys2 = g_list_copy (keys);
      keys2 = g_list_sort (keys2, (void*)g_strcmp0);
      keys = keys2;

      cs = g_string_new ("<div class='categories'>");

      for (k = keys; k; k = k->next)
      {
         category = k->data;
         if (!strstr (category, "Gegl"))
         g_string_append_printf (cs, "<a href='%s.html'>%s</a> ", category, category);
      }
      g_string_append_printf (cs, "</div>");

      k = keys;
      while (k) {
        category = k->data;
all:
        g_string_assign (s, "");
        if (category)
        {
          gchar *title = g_strdup_printf ("GEGL %s operations", category);
          g_string_append_printf (s, html_pre, title, css, " body{max-width:98%;} ");
          g_free (title);
          g_string_append_printf (s, "<h2 style='clear:both;'>GEGL %s operations</h2>\n", category);
          g_string_append_printf (s, "%s", cs->str);

          for (iter=operations;iter;iter = g_list_next (iter))
          {
            GeglOperationClass *klass = iter->data;
            const char *name = gegl_operation_class_get_key (klass, "name");
            const char *description = gegl_operation_class_get_key (klass, "description");
            const char *categoris = gegl_operation_class_get_key (klass, "categories");
            int found = 0;
      if (categoris)
      {
        const gchar *ptr = categoris;
        while (ptr && *ptr)
          {
            gchar categry[64]="";
            gint i=0;
            while (*ptr && *ptr!=':' && i<63)
              {
                categry[i++]=*(ptr++);
                categry[i]='\0';
              }
            if (*ptr==':')
              ptr++;
            if (!strcmp( categry, category))
              found = 1;
          }
      }
      if (found || !strcmp(category, "all") ||
        !strcmp(category, g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass))) ))
      {
        gchar *name_dup = g_strdup (name);
        gchar *colon = strchr (name_dup, ':');
        if (colon) *colon = '-';

      {
        char *image = operation_to_image_path (name);
        if (!g_file_test (image, G_FILE_TEST_EXISTS))
        {
           g_free (image);
           image = g_strdup ("images/gegl-ditto.png");
        }
        g_string_append_printf (s, "<a href='%s.html' title='", name_dup);
        if (description)
          xml_escape_string (s, description);
        g_string_append_printf (s, "'><div style='display:inline-block;width:200px;height:200px;background-image:url(%s);background-repeat: no-repeat;padding-bottom:2px;color:white;text-shadow:1px 1px 2px black;'>", image);
        g_string_append_printf (s, "%s</div></a>\n", name);
        g_free (name_dup);
        g_free (image);
      }
      }
          }
        }
        g_string_append (s, html_post);

      {
        gchar *html_name = g_strdup_printf ("%s.html", category);
        g_print ("%s\n", html_name);
        g_file_set_contents (html_name, s->str, -1, NULL);
        g_free (html_name);
      }
        if (k)
          k = k->next;
        if (!k && strcmp(category, "all"))
        {
          category="all";
          goto all;
        }
      }
   }
   if(1){
      gchar *category = "all";
      GList *keys = g_hash_table_get_keys (seen_categories);
      GList *keys2 = g_list_copy (keys);
      GList *k;
      GString *cs;
      keys2 = g_list_sort (keys2, (void*)g_strcmp0);
      keys = keys2;
      k = keys;

      cs = g_string_new ("<div class='categories'>");

      for (k = keys; k; k = k->next)
      {
         category = k->data;
         if (!strstr (category, "Gegl"))
           g_string_append_printf (cs, "<a href='%s.html'>%s</a> ", category, category);
      }
      g_string_append_printf (cs, "</div>");



        g_string_assign (s, "");
        {
          gchar *title = g_strdup_printf ("GEGL operations");
          g_string_append_printf (s, html_pre, title, css, "");
          g_free (title);
          g_string_append_printf (s, "<div style='margin-top:3em;'><a href='../index.html'><img src='../images/GEGL.png' alt='GEGL' style='height: 6.0em;float:left; padding-right:0.5em;'/></a><h2> All GEGL operations</h2><p>This part of the GEGL documentation contains a snapshot of reference rendering images and meta-data, useful for programming with GEGL as well as used by GIMP for automatically constructing property panels user interfaces. The tags in on the right lead to galleries of ops belonging to each category.</p>");
          g_string_append_printf (s, "<p style='float:right;'>categories:</p>%s\n", cs->str);

          for (iter=operations;iter;iter = g_list_next (iter))
          {
            GeglOperationClass *klass = iter->data;
            const char *name = gegl_operation_class_get_key (klass, "name");
            const char *title = gegl_operation_class_get_key (klass, "title");
            const char *description = gegl_operation_class_get_key (klass, "description");
      {
        gchar *name_dup = g_strdup (name);
        gchar *colon = strchr (name_dup, ':');
        if (colon) *colon = '-';

      {
        char *image = operation_to_image_path (name);
        if (!g_file_test (image, G_FILE_TEST_EXISTS))
        {
           g_free (image);
           image = g_strdup ("images/gegl-ditto.png");
        }
        g_string_append_printf (s, "<a href='%s.html' title='", name_dup);
        if (description)
          xml_escape_string (s, description);
        g_string_append_printf (s, "'><div style='display: box-inline;width: 20em; overflow: hidden;'>");
        g_string_append_printf (s, "%s</div></a>\n", title?title:name);
        g_free (name_dup);
        g_free (image);
      }
      }
          }
        }
        g_string_append_printf (s, "</div></body></html>");

      {
        gchar *html_name = "index.html";
        g_print ("%s\n", html_name);
        g_file_set_contents (html_name, s->str, -1, NULL);
      }
        k = k->next;
   }

  return 0;
}
