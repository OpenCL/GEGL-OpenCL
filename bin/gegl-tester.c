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
 */

#include <glib.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <string.h>
#include <glib/gprintf.h>


static GRegex   *regex, *exc_regex;
static gchar    *data_dir        = NULL;
static gchar    *reference_dir   = NULL;
static gchar    *output_dir      = NULL;
static gchar    *pattern         = "";
static gchar    *exclusion_pattern = "a^"; /* doesn't match anything by default */
static gboolean *output_all      = FALSE;

static const GOptionEntry options[] =
{
  {"data-directory", 'd', 0, G_OPTION_ARG_FILENAME, &data_dir,
   "Root directory of files used in the composition", NULL},

  {"reference-directory", 'r', 0, G_OPTION_ARG_FILENAME, &reference_dir,
   "Directory where reference images are located", NULL},

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

static gboolean
test_operation (const gchar *op_name,
                const gchar *image,
                gchar       *output_path)
{
  gchar         *ref_path;
  GeglNode      *img, *ref_img, *gegl;
  GeglRectangle  ref_bounds, comp_bounds;
  gint           ref_pixels;
  gboolean       result = TRUE;

  gegl = gegl_node_new ();

  ref_path = g_build_path (G_DIR_SEPARATOR_S, reference_dir, image, NULL);
  ref_img = gegl_node_new_child (gegl,
                                 "operation", "gegl:load",
                                 "path", ref_path,
                                 NULL);
  g_free (ref_path);

  img = gegl_node_new_child (gegl,
                             "operation", "gegl:load",
                             "path", output_path,
                             NULL);

  ref_bounds  = gegl_node_get_bounding_box (ref_img);
  comp_bounds = gegl_node_get_bounding_box (img);
  ref_pixels  = ref_bounds.width * ref_bounds.height;

  if (ref_bounds.width != comp_bounds.width ||
      ref_bounds.height != comp_bounds.height)
    {
      g_printf ("FAIL\n  Reference and composition differ in size\n");
      result = FALSE;
    }
  else
    {
      GeglNode *comparison;
      gdouble   max_diff;

      comparison = gegl_node_create_child (gegl, "gegl:image-compare");
      gegl_node_link (img, comparison);
      gegl_node_connect_to (ref_img, "output", comparison, "aux");
      gegl_node_process (comparison);
      gegl_node_get (comparison, "max diff", &max_diff, NULL);

      if (max_diff < 1.0)
        {
          g_printf ("PASS\n");
          result = TRUE;
        }
      else
        {
          GeglNode *output;
          gchar    *diff_path;
          gdouble   avg_diff_wrong, avg_diff_total;
          gint      wrong_pixels;

          gegl_node_get (comparison, "avg_diff_wrong", &avg_diff_wrong,
                         "avg_diff_total", &avg_diff_total, "wrong_pixels",
                         &wrong_pixels, NULL);

          g_printf ("FAIL\n  Reference image and composition differ\n"
                    "    wrong pixels : %i/%i (%2.2f%%)\n"
                    "    max Δe       : %2.3f\n"
                    "    avg Δe       : %2.3f (wrong) %2.3f (total)\n",
                    wrong_pixels, ref_pixels,
                    (wrong_pixels * 100.0 / ref_pixels),
                    max_diff, avg_diff_wrong, avg_diff_total);

          diff_path = operation_to_path (op_name, TRUE);
          output = gegl_node_new_child (gegl,
                                        "operation", "gegl:png-save",
                                        "path", diff_path,
                                        NULL);
          gegl_node_link (comparison, output);
          gegl_node_process (output);

          g_free (diff_path);

          result = FALSE;
        }
    }

  g_object_unref (gegl);
  return result;
}

static void
standard_output (const gchar *op_name)
{
  GeglNode *composition, *input, *aux, *operation, *crop, *output;
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

      gegl_node_link_many (operation, crop, output, NULL);

      if (gegl_node_has_pad (operation, "input"))
        gegl_node_link (input, operation);

      if (gegl_node_has_pad (operation, "aux"))
        gegl_node_connect_to (aux, "output", operation, "aux");

      gegl_node_process (output);
    }

  g_free (input_path);
  g_free (aux_path);
  g_free (output_path);
  g_object_unref (composition);
}

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
      gboolean            matches;

      operation_class = g_type_class_ref (operations[i]);
      image           = gegl_operation_class_get_key (operation_class, "reference-image");
      xml             = gegl_operation_class_get_key (operation_class, "reference-composition");
      name            = gegl_operation_class_get_key (operation_class, "name");

      if (name == NULL)
        {
          result = result && process_operations (operations[i]);
          continue;
        }

      matches = g_regex_match (regex, name, 0, NULL) &&
        !g_regex_match (exc_regex, name, 0, NULL);

      if (xml && matches)
        {
          GeglNode *composition;

          if (output_all)
            g_printf ("%s\n", name);
          else if (image)
            g_printf ("%s: ", name); /* more information will follow
                                        if we're testing */

          composition = gegl_node_new_from_xml (xml, data_dir);
          if (!composition)
            {
              g_printf ("FAIL\n  Composition graph is flawed\n");
              result = FALSE;
            }
          else if (image || output_all)
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

              /* don't test if run with --all */
              if (!output_all && image)
                result = test_operation (name, image, output_path) && result;

              g_free (output_path);
            }
        }
      /* if we are running with --all and the operation doesn't have a
         composition, use standard composition and images, don't test */
      else if (output_all && matches &&
               !(g_type_is_a (operations[i], GEGL_TYPE_OPERATION_SINK) ||
                 g_type_is_a (operations[i], GEGL_TYPE_OPERATION_TEMPORAL)))
        {
          g_printf ("%s\n", name);
          standard_output (name);
        }

      result = process_operations (operations[i]) && result;
    }

  g_free (operations);

  return result;
}

gint
main (gint    argc,
      gchar **argv)
{
  gboolean        result;
  GError         *error = NULL;
  GOptionContext *context;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gegl_get_option_group ());

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printf ("%s\n", error->message);
      g_error_free (error);
      result = FALSE;
    }
  else if (output_all && !(data_dir && output_dir))
    {
      g_printf ("Data and output directories must be specified\n");
      result = FALSE;
    }
  else if (!(output_all || (data_dir && output_dir && reference_dir)))
    {
      g_printf ("Data, reference and output directories must be specified\n");
      result = FALSE;
    }
  else
    {
      regex = g_regex_new (pattern, 0, 0, NULL);
      exc_regex = g_regex_new (exclusion_pattern, 0, 0, NULL);

      result = process_operations (GEGL_TYPE_OPERATION);

      g_regex_unref (regex);
      g_regex_unref (exc_regex);
    }

  gegl_exit ();

  if (output_all)
    return 0;
  else
    return result;
}
