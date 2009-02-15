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

/* FIXME: this should not be neccesary to implement this operation */
GeglBuffer *gegl_node_get_cache           (GeglNode      *node);

static void
gegl_introspect_load_cache (GeglChantO *op_introspect)
{
  GeglRectangle rect = { 0, };
  GeglNode     *temp_gegl            = NULL;
  gchar        *dot_string           = NULL;
  gchar        *png_filename         = NULL;
  gchar        *escaped_png_filename = NULL;
  gchar        *dot_filename         = NULL;
  gchar        *xml                  = NULL;
  gchar        *dot_cmd              = NULL;

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
  system (dot_cmd);

  /* Create an XML string that loads the PNG */
  escaped_png_filename = g_markup_escape_text (png_filename, -1);
  xml = g_strdup_printf ("<gegl>"
                         "  <node operation='gegl:png-load' path='%s' />"
                         "</gegl>",
                         escaped_png_filename);

  /* Create a GEGL graph from the XML */
  temp_gegl = gegl_node_new_from_xml (xml, "/");
  rect = gegl_node_get_bounding_box (temp_gegl);

  /* Force a render of the cache, passing in a NULL buffer indicating
   * that we do not actually desire the rendered data.
   */
  gegl_node_blit (temp_gegl, 1.0, &rect, NULL, NULL, 0, GEGL_BLIT_CACHE);

  /* Get hold of a GeglBuffer with the data */
  {
    GeglBuffer *cache  = GEGL_BUFFER (gegl_node_get_cache (temp_gegl));
    GeglBuffer *newbuf = gegl_buffer_create_sub_buffer (cache, &rect);
    op_introspect->chant_data = (gpointer) newbuf;
    g_object_unref (cache);
  }

  /* Cleanup */
  g_object_unref (temp_gegl);
  g_free (dot_string);
  g_free (dot_cmd);
  g_free (dot_filename);
  g_free (png_filename);
  g_free (escaped_png_filename);
  g_free (xml);
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
                         const GeglRectangle  *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (!o->chant_data)
    return FALSE;

  /* overriding the predefined behavior */
  gegl_operation_context_set_object (context, "output", G_OBJECT (o->chant_data));
  g_object_unref (o->chant_data);
  o->chant_data = NULL;

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  operation_class->process          = gegl_introspect_process;
  operation_class->get_bounding_box = gegl_introspect_get_bounding_box;

  operation_class->name        = "gegl:introspect";
  operation_class->categories  = "render";
  operation_class->description = _("GEGL graph visualizer.");
}

#endif
