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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <stdlib.h>
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_object(node, _("Node"), _("GeglNode to introspect"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "introspect.c"

#include "gegl-chant.h"
#include "gegl-dot.h" /* XXX: internal header file */
#include <stdio.h>


static void
gegl_introspect_load_cache (GeglChantO *op_introspect)
{
  GeglBuffer *new_buffer   = NULL;
  GeglNode   *png_load     = NULL;
  GeglNode   *buffer_sink  = NULL;
  gchar      *dot_string   = NULL;
  gchar      *png_filename = NULL;
  gchar      *dot_filename = NULL;
  gchar      *dot_cmd      = NULL;

  if (op_introspect->chant_data)
    return;

  /* Construct temp filenames */
  dot_filename = g_build_filename (g_get_tmp_dir (), "gegl-introspect.dot", NULL);
  png_filename = g_build_filename (g_get_tmp_dir (), "gegl-introspect.png", NULL);

  /* Construct the .dot source */
  dot_string = gegl_to_dot (GEGL_NODE (op_introspect->node));
  g_file_set_contents (dot_filename, dot_string, -1, NULL);

  /* Process the .dot to a .png */
  dot_cmd = g_strdup_printf ("dot -o %s -Tpng %s", png_filename, dot_filename);
  if (system (dot_cmd) == -1)
    g_warning ("Error executing GraphViz dot program");

  /* Create a graph that loads the png into a GeglBuffer and process
   * it
   */
  png_load = gegl_node_new_child (NULL,
                                  "operation", "gegl:png-load",
                                  "path",      png_filename,
                                  NULL);
  buffer_sink = gegl_node_new_child (NULL,
                                     "operation", "gegl:buffer-sink",
                                     "buffer",    &new_buffer,
                                     NULL);
  gegl_node_link_many (png_load, buffer_sink, NULL);
  gegl_node_process (buffer_sink);

  op_introspect->chant_data= new_buffer;

  /* Cleanup */
  g_object_unref (buffer_sink);
  g_object_unref (png_load);
  g_free (dot_string);
  g_free (dot_cmd);
  g_free (dot_filename);
  g_free (png_filename);
}

static void
gegl_introspect_dispose (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data != NULL)
    {
      g_object_unref (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->dispose (object);
}

static GeglRectangle
gegl_introspect_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  gint width, height;

  gegl_introspect_load_cache (o);

  g_object_get (o->chant_data, "width", &width,
                               "height", &height, NULL);

  result.width  = width;
  result.height = height;

  return result;
}

static gboolean
gegl_introspect_process (GeglOperation        *operation,
                         GeglOperationContext *context,
                         const gchar          *output_pad,
                         const GeglRectangle  *result,
                         gint                  level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_introspect_load_cache (o);

  /* gegl_operation_context_take_object() takes the reference we have,
   * so we must increase it since we want to keep the object
   */
  g_object_ref (o->chant_data);

  gegl_operation_context_take_object (context, output_pad, G_OBJECT (o->chant_data));

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose             = gegl_introspect_dispose;

  operation_class->process          = gegl_introspect_process;
  operation_class->get_bounding_box = gegl_introspect_get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:introspect",
    "categories" , "render",
    "description", _("GEGL graph visualizer."),
    NULL);
}

#endif
