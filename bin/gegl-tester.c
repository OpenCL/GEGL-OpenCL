/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Ville Sokk <ville.sokk@gmail.com>
 *           2003, 2004, 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include <glib.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <string.h>


#define match(string) (!strcmp (*curr, (string)))

#define assert_argument() do {\
    if (!curr[1] || curr[1][0]=='-') {\
        g_printerr ("ERROR: '%s' option expected argument\n", *curr);\
        exit(-1);\
    }\
}while(0)

#define get_string(var) do{\
    assert_argument();\
    curr++;\
    (var)=*curr;\
}while(0)


static GRegex *regex;
static gchar  *data_dir;
static gchar  *reference_dir;
static gchar  *output_dir;


static gboolean
process_operations (GType type)
{
  GType    *operations;
  gboolean  result = TRUE;
  guint     count;
  gint      i;

  operations = g_type_children (type, &count);

  if (!operations)
    {
      g_free (operations);
      return TRUE;
    }

  for (i = 0; i < count; i++)
    {
      GeglOperationClass *operation_class;
      const gchar        *image, *xml, *name;

      operation_class = g_type_class_ref (operations[i]);
      image           = gegl_operation_class_get_key (operation_class, "reference-image");
      xml             = gegl_operation_class_get_key (operation_class, "reference-composition");
      name            = gegl_operation_class_get_key (operation_class, "name");

      if (image && xml && g_regex_match (regex, name, 0, NULL))
        {
          gchar    *image_path  = g_build_path (G_DIR_SEPARATOR_S, reference_dir, image, NULL);
          gchar    *output_path = g_build_path (G_DIR_SEPARATOR_S, output_dir, image, NULL);
          GeglNode *composition, *output, *ref_img, *comparison;
          gdouble   max_diff;

          g_printf ("%s: ", name);

          composition = gegl_node_new_from_xml (xml, data_dir);
          if (!composition)
            {
              g_printerr ("\nComposition graph is flawed\n");
              result = FALSE;
            }
          else
            {
              output = gegl_node_new_child (composition,
                                            "operation", "gegl:save",
                                            "path", output_path,
                                            NULL);
              gegl_node_link_many (composition, output, NULL);
              gegl_node_process (output);

              ref_img = gegl_node_new_child (composition,
                                             "operation", "gegl:load",
                                             "path", image_path,
                                             NULL);

              comparison = gegl_node_create_child (composition, "gegl:image-compare");

              gegl_node_link_many (composition, comparison, NULL);
              gegl_node_connect_to (ref_img, "output", comparison, "aux");
              gegl_node_process (comparison);
              gegl_node_get (comparison, "max diff", &max_diff, NULL);

              if (max_diff < 1.5)
                {
                  g_printf ("PASS\n");
                  result = result && TRUE;
                }
              else
                {
                  gint   img_length = strlen (image);
                  gchar *diff_file  = g_malloc (img_length + 16);
                  gint   ext_length = strlen (strrchr (image, '.'));

                  memcpy (diff_file, image, img_length + 1);
                  memcpy (diff_file + img_length - ext_length, "-diff.png", 11);

                  g_free (output_path);
                  output_path = g_build_path (G_DIR_SEPARATOR_S, output_dir, diff_file, NULL);
                  gegl_node_set (output, "path", output_path, NULL);
                  gegl_node_link_many (comparison, output, NULL);
                  gegl_node_process (output);

                  g_printf ("FAIL\n");
                  result = result && FALSE;

                  g_free (diff_file);
                }
            }

          g_object_unref (composition);
          g_free (image_path);
          g_free (output_path);
        }

      result = result && process_operations(operations[i]);
    }

  g_free (operations);

  return result;
}

gint
main (gint    argc,
      gchar **argv)
{
  gint    result;
  gchar  *pattern = "";
  gchar **curr    = argv;
  gchar  *cwd     = g_get_current_dir ();

  reference_dir = cwd;
  output_dir    = cwd;
  data_dir      = cwd;

  while (*curr)
    {
      if (match ("--data") || match ("-d"))
        get_string (data_dir);
      else if (match ("--reference") || match ("-r"))
        get_string (reference_dir);
      else if (match ("--output") || match ("-o"))
        get_string (output_dir);
      else if (match ("--pattern") || match ("-p"))
        get_string (pattern);
      curr++;
    }

  g_thread_init (NULL);
  gegl_init (&argc, &argv);

  regex = g_regex_new (pattern, 0, 0, NULL);

  result = process_operations (GEGL_TYPE_OPERATION);

  g_regex_unref (regex);
  g_free (cwd);
  gegl_exit ();

  return result;
}
