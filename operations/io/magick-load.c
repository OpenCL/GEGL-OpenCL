/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
gegl_chant_path (path, "/tmp/gegl-logo.svg", "Path of file to load.")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            magick_load
#define GEGL_CHANT_DESCRIPTION     "Image Magick wrapper using the png op."

#define GEGL_CHANT_SELF            "magick-load.c"
#define GEGL_CHANT_CATEGORIES      "hidden"
#include "gegl-chant.h"
#include <stdio.h>

static void
load_cache (GeglChantOperation *op_magick_load)
{
  if (!op_magick_load->priv)
    {
        GeglRectangle rect;
        GeglNode *temp_gegl;
        gchar xml[1024]="";
          {
            /* ImageMagick backed fallback FIXME: make this robust.
             * maybe use pipes in a manner similar to the raw loader */
            gchar cmd_buf[1024];
            sprintf (cmd_buf, "convert \"%s\"'[0]' /tmp/gegl-magick.png", op_magick_load->path);
            system (cmd_buf);

            sprintf (xml, "<gegl><node operation='png-load' path='/tmp/gegl-magick.png'></node></gegl>");
          }

    temp_gegl = gegl_parse_xml (xml, "/");
    rect = gegl_node_get_bounding_box (temp_gegl);

    gegl_node_blit (temp_gegl, &rect, 1.0, NULL, 0, NULL, GEGL_BLIT_CACHE); /* force a render
                                                                               of the cache,
                                                                               passing in a NULL
                                                                               buffer indicating that
                                                                               we do not actually desire the rendered data
                                                                               */

    {
      GeglBuffer *cache = GEGL_BUFFER(gegl_node_get_cache (temp_gegl));
      GeglBuffer *newbuf = gegl_buffer_create_sub_buffer (cache, &rect);
      op_magick_load->priv = (gpointer)newbuf;
      g_object_unref (cache);
    }
    /*g_object_ref (op_magick_load->priv);*/

    /*FIXME: this should be unneccesary, using the graph
     * directly as a node is more elegant.
     */
    /*gegl_node_get (temp_gegl, "output", &(op_magick_load->priv), NULL);*/
    g_object_unref (temp_gegl);
  }
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  if (!self->priv)
    return FALSE;

  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (self->priv));
  self->priv = NULL;

  return  TRUE;
}


static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  gint width, height;

  load_cache (self);

  g_object_get (self->priv, "width", &width,
                            "height", &height, NULL);

  result.width  = width;
  result.height  = height;

  return result;
}

#endif
