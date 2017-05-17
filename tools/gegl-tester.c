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
 *           2017 Øyvind Kolås <pippin@gimp.org>
 */

#include <glib.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <locale.h>
#include <string.h>
#include <glib/gprintf.h>

static GRegex   *regex, *exc_regex;
static gchar    *data_dir        = NULL;
static gchar    *output_dir      = NULL;
static gchar    *pattern         = "";
static gchar    *exclusion_pattern = "a^"; /* doesn't match anything by default */
static gboolean *output_all      = FALSE;
static gint      failed          = 0;
static GString  *failed_ops      = NULL;

static const GOptionEntry options[] =
{
  {"data-directory", 'd', 0, G_OPTION_ARG_FILENAME, &data_dir,
   "Root directory of files used in the composition", NULL},

  {"output-directory", 'o', 0, G_OPTION_ARG_FILENAME, &output_dir,
   "Directory where composition output and diff files are saved", NULL},

  {"pattern", 'p', 0, G_OPTION_ARG_STRING, &pattern,
   "Regular expression used to match names of operations to be tested", NULL},

  {"exclusion-pattern", 'e', 0, G_OPTION_ARG_STRING, &exclusion_pattern,
   "Regular expression used to match names of operations not to be tested", NULL},

  {"all", 'a', 0, G_OPTION_ARG_NONE, &output_all,
   "Create output for all operations using a standard composition "
   "if no composition is specified", NULL},

  { NULL }
};

/* convert operation name to output path */
static gchar*
operation_to_path (const gchar *op_name,
                   gboolean     diff)
{
  gchar *cleaned = g_strdup (op_name);
  gchar *filename, *output_path;

  g_strdelimit (cleaned, ":", '-');
  if (diff)
    filename = g_strconcat (cleaned, "-diff.png", NULL);
  else
    filename = g_strconcat (cleaned, ".png", NULL);
  output_path = g_build_path (G_DIR_SEPARATOR_S, output_dir, filename, NULL);

  g_free (cleaned);
  g_free (filename);

  return output_path;
}

static void
standard_output (const gchar *op_name)
{
  GeglNode *composition, *input, *aux, *operation, *crop, *output, *translate;
  GeglNode *background,  *over;
  gchar    *input_path  = g_build_path (G_DIR_SEPARATOR_S, data_dir,
                                        "standard-input.png", NULL);
  gchar    *aux_path    = g_build_path (G_DIR_SEPARATOR_S, data_dir,
                                        "standard-aux.png", NULL);
  gchar    *output_path = operation_to_path (op_name, FALSE);

  composition = gegl_node_new ();
  operation = gegl_node_create_child (composition, op_name);

  if (gegl_node_has_pad (operation, "output"))
    {
      input = gegl_node_new_child (composition,
                                   "operation", "gegl:load",
                                   "path", input_path,
                                   NULL);
      translate  = gegl_node_new_child (composition,
                                 "operation", "gegl:translate",
                                 "x", 0.0,
                                 "y", 80.0,
                                 NULL);
      aux = gegl_node_new_child (composition,
                                 "operation", "gegl:load",
                                 "path", aux_path,
                                 NULL);
      crop = gegl_node_new_child (composition,
                                  "operation", "gegl:crop",
                                  "width", 200.0,
                                  "height", 200.0,
                                  NULL);
      output = gegl_node_new_child (composition,
                                    "operation", "gegl:png-save",
                                    "compression", 9,
                                    "path", output_path,
                                    NULL);
      background = gegl_node_new_child (composition,
                                        "operation", "gegl:checkerboard",
                                        "color1", gegl_color_new ("rgb(0.75,0.75,0.75)"),
                                        "color2", gegl_color_new ("rgb(0.25,0.25,0.25)"),
                                        NULL);
      over = gegl_node_new_child (composition, "operation", "gegl:over", NULL);


      if (gegl_node_has_pad (operation, "input"))
        gegl_node_link (input, operation);

      if (gegl_node_has_pad (operation, "aux"))
      {
        gegl_node_connect_to (aux, "output", translate, "input");
        gegl_node_connect_to (translate, "output", operation, "aux");
      }

      gegl_node_connect_to (background, "output", over, "input");
      gegl_node_connect_to (operation,  "output", over, "aux");
      gegl_node_connect_to (over,       "output", crop, "input");
      gegl_node_connect_to (crop,       "output", output, "input");


      gegl_node_process (output);
    }

  g_free (input_path);
  g_free (aux_path);
  g_free (output_path);
  g_object_unref (composition);
}

static gchar *
compute_hash_for_path (const gchar *path)
{
  gchar *ret = NULL;
  GeglNode *gegl = gegl_node_new ();
  GeglRectangle comp_bounds;
  guchar *buf;
  GeglNode *img = gegl_node_new_child (gegl,
                                 "operation", "gegl:load",
                                 "path", path,
                                 NULL);
  comp_bounds = gegl_node_get_bounding_box (img);
  buf = g_malloc0 (comp_bounds.width * comp_bounds.height * 4);
  gegl_node_blit (img, 1.0, &comp_bounds, babl_format("R'G'B'A u8"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
  ret = g_compute_checksum_for_data (G_CHECKSUM_MD5, buf, comp_bounds.width * comp_bounds.height * 4);
  g_free (buf);
  g_object_unref (gegl);
  return ret;
}

static void
process_operations (GType type)
{
  GType    *operations;
  guint     count;
  gint      i;

  operations = g_type_children (type, &count);

  if (!operations)
    {
      g_free (operations);
      return;
    }

  for (i = 0; i < count; i++)
    {
      GeglOperationClass *operation_class;
      const gchar        *xml, *name, *hash, *hashB, *hashC, *chain;
      gboolean            matches;

      operation_class = g_type_class_ref (operations[i]);
      hash            = gegl_operation_class_get_key (operation_class, "reference-hash");
      hashB           = gegl_operation_class_get_key (operation_class, "reference-hashB");
      hashC           = gegl_operation_class_get_key (operation_class, "reference-hashC");
      xml             = gegl_operation_class_get_key (operation_class, "reference-composition");
      chain           = gegl_operation_class_get_key (operation_class, "reference-chain");
      name            = gegl_operation_class_get_key (operation_class, "name");

      if (name == NULL)
        {
          process_operations (operations[i]);
          continue;
        }

      matches = g_regex_match (regex, name, 0, NULL) &&
        !g_regex_match (exc_regex, name, 0, NULL);

      if ((chain||xml) && matches)
        {
          GeglNode *composition;

          g_printf ("%s: ", name); /* more information will follow
                                        if we're testing */

          if (xml)
            composition = gegl_node_new_from_xml (xml, data_dir);
          else
            composition = gegl_node_new_from_serialized (chain, data_dir);

          if (!composition)
            {
              g_printf ("FAIL\n  Composition graph is flawed\n");
            }
          else if (output_all)
            {
              gchar    *output_path = operation_to_path (name, FALSE);
              GeglNode *output      =
                gegl_node_new_child (composition,
                                     "operation", "gegl:png-save",
                                     "compression", 9,
                                     "path", output_path,
                                     NULL);
              gegl_node_link (composition, output);
              gegl_node_process (output);
              g_object_unref (composition);

              g_free (output_path);
            }
        }
      /* if we are running with --all and the operation doesn't have a
         composition, use standard composition and images, don't test */
      else if (output_all && matches &&
               !(g_type_is_a (operations[i], GEGL_TYPE_OPERATION_SINK) ||
                 g_type_is_a (operations[i], GEGL_TYPE_OPERATION_TEMPORAL)))
        {
          g_printf ("%s ", name);
          standard_output (name);
        }

      if (matches && hash)
      {
        gchar *output_path = operation_to_path (name, FALSE);
        gchar *gothash = compute_hash_for_path (output_path);
        if (g_str_equal (hash, gothash))
          g_printf (" OK\n");
        else if (hashB && g_str_equal (hashB, gothash))
          g_printf (" OK (hash b)\n");
        else if (hashC && g_str_equal (hashC, gothash))
          g_printf (" OK (hash c)\n");
        else
        {
          g_printf (" FAIL %s != %s\n", gothash, hash);
          failed ++;
          g_string_append_printf (failed_ops, "%s %s != %s\n", name, gothash, hash);
        }
        g_free (gothash);
        g_free (output_path);
      } else if (matches &&
               !(g_type_is_a (operations[i], GEGL_TYPE_OPERATION_SINK) ||
                 g_type_is_a (operations[i], GEGL_TYPE_OPERATION_TEMPORAL)))
      {
        gchar *output_path = operation_to_path (name, FALSE);
        gchar *gothash = compute_hash_for_path (output_path);
        if (g_str_equal (gothash, "9bbe341d798da4f7b181c903e6f442fd"))
          g_printf (" reference is noop?\n");
        else
          g_printf (" hash = %s\n", gothash);
        g_free (gothash);
        g_free (output_path);
      }

      process_operations (operations[i]);
    }

  g_free (operations);
}

gint
main (gint    argc,
      gchar **argv)
{
  GError         *error = NULL;
  GOptionContext *context;

  setlocale (LC_ALL, "");


  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gegl_get_option_group ());

  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);
  failed_ops = g_string_new ("");

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printf ("%s\n", error->message);
      g_error_free (error);
      return -1;
    }
  else if (output_all && !(data_dir && output_dir))
    {
      g_printf ("Data and output directories must be specified\n");
      return -1;
    }
  else if (!(output_all || (data_dir && output_dir)))
    {
      g_printf ("Data and output directories must be specified\n");
      return -1;
    }
  else
    {

      regex = g_regex_new (pattern, 0, 0, NULL);
      exc_regex = g_regex_new (exclusion_pattern, 0, 0, NULL);

      process_operations (GEGL_TYPE_OPERATION);

      g_regex_unref (regex);
      g_regex_unref (exc_regex);
    }

  gegl_exit ();

  g_printf ("\n\nwith%s opencl acceleration\n", gegl_cl_is_accelerated()?"":"out");
  if (failed != 0)
  {

    g_print ("Maybe see bug https://bugzilla.gnome.org/show_bug.cgi?id=780226\n%i operations producing unexpected hashes:\n%s\n", failed, failed_ops->str);
    return -1;
  }
  g_string_free (failed_ops, TRUE);

  return 0;
}
