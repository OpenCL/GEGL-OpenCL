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

static void
load_cache (GeglChantO *op_magick_load)
{
  if (!op_magick_load->chant_data)
    {
      gchar    *filename;
      gchar    *cmd;
      GeglNode *graph, *sink;
      GeglBuffer *newbuf = NULL;

      /* ImageMagick backed fallback FIXME: make this robust.
       * maybe use pipes in a manner similar to the raw loader,
       * or at least use a properly unique filename  */

      filename = g_build_filename (g_get_tmp_dir (), "gegl-magick.png", NULL);
      cmd = g_strdup_printf ("convert \"%s\"'[0]' \"%s\"",
                             op_magick_load->path, filename);
      if (system (cmd) == -1)
        g_warning ("Error executing ImageMagick convert program");

      graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &newbuf, NULL,
                                          gegl_node ("gegl:png-load", "path", filename, NULL)));
      gegl_node_process (sink);
      op_magick_load->chant_data = (gpointer) newbuf;
      g_object_unref (graph);
      g_free (cmd);
      g_free (filename);
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

static GeglRectangle
get_cached_region (GeglOperation *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static gboolean
process (GeglOperation         *operation,
         GeglOperationContext  *context,
         const gchar           *output_pad,
         const GeglRectangle   *result,
         gint                   level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (!o->chant_data)
    return FALSE;
  /* overriding the predefined behavior */
  g_object_ref (o->chant_data);
  gegl_operation_context_take_object (context, "output", G_OBJECT (o->chant_data));
  return  TRUE;
}

static void finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  if (o->chant_data)
    g_object_unref (o->chant_data);
  o->chant_data = NULL;
  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GObjectClass             *object_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  object_class    = G_OBJECT_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;;
  operation_class->no_cache = FALSE;

  gegl_operation_class_set_keys (operation_class,
        "name"       , "gegl:magick-load",
        "categories" , "hidden",
        "description",
        _("Image Magick wrapper using the png op."),
        NULL);
}

#endif
