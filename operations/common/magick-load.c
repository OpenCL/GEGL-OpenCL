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

gegl_chant_file_path (path, _("File"), "/tmp/gegl-logo.svg", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "magick-load.c"

#include "gegl-chant.h"
#include <stdio.h>

/* FIXME: this should not be neccesary to implement this operation */
GeglBuffer *gegl_node_get_cache           (GeglNode      *node);

static void
load_cache (GeglChantO *op_magick_load)
{
  if (!op_magick_load->chant_data)
    {
      GeglRectangle rect;
      GeglNode *temp_gegl;
      gchar    *filename;
      gchar    *escaped;
      gchar    *xml;
      gchar    *cmd;

      /* ImageMagick backed fallback FIXME: make this robust.
       * maybe use pipes in a manner similar to the raw loader */

      filename = g_build_filename (g_get_tmp_dir (), "gegl-magick.png", NULL);
      cmd = g_strdup_printf ("convert \"%s\"'[0]' \"%s\"",
                             op_magick_load->path, filename);
      system (cmd);
      g_free (cmd);

      escaped = g_markup_escape_text (filename, -1);
      g_free (filename);

      xml = g_strdup_printf ("<gegl>"
                             "<node operation='gegl:png-load' path='%s' />"
                             "</gegl>",
                             escaped);
      g_free (escaped);

      temp_gegl = gegl_node_new_from_xml (xml, "/");
      g_free (xml);

      rect = gegl_node_get_bounding_box (temp_gegl);

      /* Force a render of the cache, passing in a NULL buffer indicating
       * that we do not actually desire the rendered data.
       */
      gegl_node_blit (temp_gegl, 1.0, &rect, NULL, NULL, 0, GEGL_BLIT_CACHE);

      {
        GeglBuffer *cache  = GEGL_BUFFER (gegl_node_get_cache (temp_gegl));
        GeglBuffer *newbuf = gegl_buffer_create_sub_buffer (cache, &rect);
        op_magick_load->chant_data = (gpointer) newbuf;
        g_object_unref (cache);
      }
      /*g_object_ref (op_magick_load->chant_data);*/

      /*FIXME: this should be unneccesary, using the graph
       * directly as a node is more elegant.
       */
      /*gegl_node_get (temp_gegl, "output", &(op_magick_load->chant_data), NULL);*/
      g_object_unref (temp_gegl);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  gint width, height;

  load_cache (o);

  g_object_get (o->chant_data, "width", &width,
                               "height", &height, NULL);

  result.width  = width;
  result.height = height;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglOperationContext     *context,
         const gchar         *output_pad,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (!o->chant_data)
    return FALSE;

  /* overriding the predefined behavior */
  gegl_operation_context_take_object (context, "output", G_OBJECT (o->chant_data));
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

  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->name        = "gegl:magick-load";
  operation_class->categories  = "hidden";
  operation_class->description =
        _("Image Magick wrapper using the png op.");
}

#endif
