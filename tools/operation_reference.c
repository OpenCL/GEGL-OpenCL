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
operation_to_path (const gchar *op_name)
{
  gchar *cleaned = g_strdup (op_name);
  gchar *filename, *output_path;

  g_strdelimit (cleaned, ":", '-');
  filename = g_strconcat (cleaned, ".png", NULL);
  output_path = g_build_path (G_DIR_SEPARATOR_S, "images", "examples", filename, NULL);

  g_free (cleaned);
  g_free (filename);

  return output_path;
}

static void json_escape_string (const char *description)
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
        g_print ("\\%c", *p);
        break;
      case '\n':
        g_print ("\\n");
        break;
      default:
        g_print ("%c", *p);
    }
  }
}

static void
json_list_pads (GType type, const gchar *opname)
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

  if (input_pads && input_pads[0])
  {
    gboolean first = TRUE;
    g_print (" ,'input-pads':[\n");
    for (i = 0; input_pads[i]; i++)
    {
      if (first)
      {
        first = FALSE;
      }
      else
      g_print (",");

      g_print ("'%s'\n", input_pads[i]);
    }
    g_print (" ]");
    g_free (input_pads);
  }

  if (output_pads && output_pads[0])
  {
    gboolean first = TRUE;
    g_print (" ,'output-pads':[\n");
    for (i = 0; output_pads[i]; i++)
    {
      if (first)
      {
        first = FALSE;
      }
      else
      g_print (",");

      g_print ("'%s'\n", output_pads[i]);
    }
    g_print (" ]");
    g_free (output_pads);
  }

  g_object_unref (gegl);
}

static void
json_list_properties (GType type, const gchar *opname)
{
  GParamSpec **self;
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;
  gboolean first_prop = TRUE;

  g_print (",'properties':[\n");

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

          if (first_prop)
          {
            first_prop = FALSE;
            g_print(" { 'name':'%s'\n", g_param_spec_get_name (self[prop_no]));
          }
          else
            g_print(",{'name':'%s'\n", g_param_spec_get_name (self[prop_no]));

          g_print("  ,'label':\"");
            json_escape_string (g_param_spec_get_nick (self[prop_no]));
          g_print ("\"\n");

          if(strstr (type_name, "Param"))
          {
            type_name = strstr (type_name, "Param");
            type_name+=5;
          }

          g_print("  ,'type':'");
          {
            for (const char *p = type_name; *p; p++)
              g_print("%c", g_ascii_tolower (*p));
          }
          g_print("'\n");

          if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_DOUBLE))
            {
              gdouble default_value = G_PARAM_SPEC_DOUBLE (self[prop_no])->default_value;
              gdouble min = G_PARAM_SPEC_DOUBLE (self[prop_no])->minimum;
              gdouble max = G_PARAM_SPEC_DOUBLE (self[prop_no])->maximum;

              if (default_value<-10000000)
                g_print ("  ,'default':'-inf'\n");
              else if (default_value>10000000)
                g_print ("  ,'default':'+inf'\n");
              else
                g_print ("  ,'default':'%2.2f'\n", default_value);

              if (min<-10000000)
                g_print ("  ,'minimum':'-inf'\n");
              else
                g_print ("  ,'minimum':'%2.2f'\n", min);

              if (max>10000000)
                g_print ("  ,'maximum':'+inf'\n");
              else
                g_print ("  ,'maximum':'%2.2f'\n", max);
              
              if (GEGL_IS_PARAM_SPEC_DOUBLE (self[prop_no]))
              {
                GeglParamSpecDouble *pspec =
                              GEGL_PARAM_SPEC_DOUBLE (self[prop_no]);

                if (pspec->ui_minimum < -10000000)
                  g_print ("  ,'ui-minimum':'-inf'\n");
                else
                  g_print ("  ,'ui-minimum':'%2.2f'\n", pspec->ui_minimum);

                if (pspec->ui_maximum > 10000000)
                  g_print ("  ,'ui-maximum':'+inf'\n");
                else
                  g_print ("  ,'ui-maximum':'%2.2f'\n", pspec->ui_maximum);

                g_print ("  ,'ui-gamma':'%2.2f'\n", pspec->ui_gamma);
                g_print ("  ,'ui-step-small':'%2.2f'\n", pspec->ui_step_small);
                g_print ("  ,'ui-step-big':'%2.2f'\n", pspec->ui_step_big);
                g_print ("  ,'ui-digits':'%i'\n", pspec->ui_digits);
              }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_INT))
            {
              gint default_value = G_PARAM_SPEC_INT (self[prop_no])->default_value;
              gint min = G_PARAM_SPEC_INT (self[prop_no])->minimum;
              gint max = G_PARAM_SPEC_INT (self[prop_no])->maximum;

              if (default_value<-10000000)
                g_print ("  ,'default':'-inf'\n");
              else if (default_value>10000000)
                g_print ("  ,'default':'+inf'\n");
              else
                g_print ("  ,'default':'%i'\n", default_value);

              if (min<-10000000)
                g_print ("  ,'minimum':'-inf'\n");
              else
                g_print ("  ,'minimum':'%i'\n", min);

              if (max>10000000)
                g_print ("  ,'maximum':'+inf'\n");
              else
                g_print ("  ,'maximum':'%i'\n", max);

              if (GEGL_IS_PARAM_SPEC_INT (self[prop_no]))
              {
                GeglParamSpecInt *pspec =
                              GEGL_PARAM_SPEC_INT (self[prop_no]);

                if (pspec->ui_minimum < -10000000)
                  g_print ("  ,'ui-minimum':'-inf'\n");
                else
                  g_print ("  ,'ui-minimum':'%i'\n", pspec->ui_minimum);

                if (pspec->ui_maximum > 10000000)
                  g_print ("  ,'ui-maximum':'+inf'\n");
                else
                  g_print ("  ,'ui-maximum':'%i'\n", pspec->ui_maximum);

                g_print ("  ,'ui-gamma':'%2.2f'\n", pspec->ui_gamma);
                g_print ("  ,'ui-step-small':'%i'\n", pspec->ui_step_small);
                g_print ("  ,'ui-step-big':'%i'\n", pspec->ui_step_big);
              }

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_BOOLEAN))
            {
              g_print ("  ,'default':'%s'\n", G_PARAM_SPEC_BOOLEAN (self[prop_no])->default_value?"True":"False");
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_STRING))
            {
              const gchar *string = G_PARAM_SPEC_STRING (self[prop_no])->default_value;

              g_print ("  ,'default':\"");
              json_escape_string (string);
              g_print ("\"\n");

            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), GEGL_TYPE_COLOR))
            {
              GeglColor *color = gegl_param_spec_color_get_default (self[prop_no]);
              if (color)
                {
                  gchar *string;

                  g_object_get (color, "string", &string, NULL);
                  g_print ("  ,'default':\"");
                  json_escape_string (string);
                  g_print ("\"\n");
                  g_free (string);
                }
            }
          else
            {
            }

          if (g_param_spec_get_blurb (self[prop_no]) &&
              g_param_spec_get_blurb (self[prop_no])[0]!='\0')
          {
            g_print ("  ,'description':\"");
          
            json_escape_string (g_param_spec_get_blurb (self[prop_no]));
            g_print ("\"\n");
          }

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
              g_print ("  ,'%s':'%s'\n",
                    property_keys[i],
                    gegl_operation_get_property_key (opname, 
                                      g_param_spec_get_name (self[prop_no]),
                                                property_keys[i]));
            }
          }
          g_free (property_keys);
        }
      }

          g_print(" }");
        }
    }
  if (self)
    g_free (self);
  if (parent)
    g_free (parent);
  g_print ("]");
}

gint
main (gint argc, gchar **argv)
{
  GList      *operations;
  GList      *iter;
  gboolean first = TRUE;

  gegl_init (&argc, &argv);

  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);


  operations = gegl_operations ();

  g_print ("window.opdb=[\n");

  for (iter=operations;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;

      const char *name = gegl_operation_class_get_key (klass, "name");
      const char *categoris = gegl_operation_class_get_key (klass, "categories");

      if (first)
        first = FALSE;
      else
        g_print (",");

      g_print ("{'op':'%s'\n", name);

      if (klass->compat_name)
        g_print (",'compat-op':'%s'\n", klass->compat_name);

      if (klass->opencl_support)
        g_print (",'opencl-support':'true'\n");

      g_print (",'parent':'%s'\n", 
          g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass))));

      {
        char *image = operation_to_path (name);

        if (g_file_test (image, G_FILE_TEST_EXISTS))
          g_print (",'image':'%s'\n", image);
        g_free (image);
      }

      {
        gchar *commandline = g_strdup_printf (
            "sh -c \"(cd " TOP_SRCDIR ";grep -r '\\\"%s\\\"' operations) | grep operations | grep -v '~:' | grep '\\\"name\\\"' | cut -f 1 -d ':'\"",
             name);
        gchar *output = NULL;
        
        if (g_spawn_command_line_sync (commandline, &output, NULL, NULL, NULL))
        {
          if (strlen(output))
            {
              output[strlen(output)-1] = 0;
              g_print (
      ",'source':'https://git.gnome.org/browse/gegl/tree/%s'\n", output);
            }
          g_free (output);
        }

        g_free (commandline);
      }

      if (categoris)
      {
        const gchar *ptr = categoris;
          gboolean first = TRUE;
        g_print (",'categories':[");

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
              if (first)
                first = FALSE;
              else
                g_print (",");
              g_print ("'%s'", category);
            }
          }
        g_print ("]\n");
      }

      json_list_properties (G_OBJECT_CLASS_TYPE (klass), name);
      json_list_pads (G_OBJECT_CLASS_TYPE (klass), name);

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
                    g_str_equal (keys[i], "source") ||
                    g_str_equal (keys[i], "name")
                    )
                  continue;

                g_print (",\"%s\":\"", keys[i]);
                json_escape_string (value);
                g_print ("\"\n");
              }
            g_free (keys);
          }
      }

      g_print (" }\n");
    }
  g_print ("]\n");

  return 0;
}
