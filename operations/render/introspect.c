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

gegl_chant_object(node, "GeglNode to introspect")
gegl_chant_pointer(buf, "Buffer")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           introspect
#define GEGL_CHANT_DESCRIPTION    "GEGL graph vizualizer."

#define GEGL_CHANT_SELF           "introspect.c"
#define GEGL_CHANT_CATEGORIES     "render"
#include "gegl-chant.h"


#include <stdio.h>
#include <string.h>
#include "gegl-plugin.h"
#include "gegl-dot.h"


static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  {
    if (self->buf)
      {
        if (context_id)
          {
            g_object_ref(self->buf); /* add an extra reference, since gegl_operation_set_data
                                        is stealing one */
            gegl_operation_set_data (operation, context_id, "output", G_OBJECT (self->buf));
          }
        return TRUE;
      }
  }
  
  {
    /*GeglRectangle *result = gegl_operation_result_rect (operation, context_id);*/

    {
      gchar *dot = gegl_to_dot ((gpointer)self->node);
      g_file_set_contents ("/tmp/gegl-temp.dot", dot, -1, NULL);
      system ("dot -o/tmp/gegl-temp.png -Tpng /tmp/gegl-temp.dot");
      g_free (dot);
    }


    {
      GeglNode *gegl = gegl_node_new ();
      GeglNode *png_load = gegl_node_new_child (gegl, "operation", "load", "path", "/tmp/gegl-temp.png", NULL);
      GeglNode *buffer_save;
      GeglRectangle defined;

      defined = gegl_node_get_bounding_box (png_load);
      self->buf = g_object_new (GEGL_TYPE_BUFFER,
                          "format", babl_format ("R'G'B' u8"),
                          "x",      defined.x,
                          "y",      defined.y,
                          "width",  defined.w,
                          "height", defined.h,
                          NULL);
      buffer_save = gegl_node_new_child (gegl, "operation", "save-buffer", "buffer", self->buf, NULL);
      gegl_node_link_many (png_load, buffer_save, NULL);

      gegl_node_process (buffer_save);
      g_object_unref (gegl);
      system ("rm /tmp/gegl-temp.*");
    }

  if (context_id)
    {
      g_object_ref (self->buf);
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (self->buf));
    }
  }
  return  TRUE;
}

static GeglRectangle 
get_defined_region (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
 
  GeglRectangle result = {0,0, 4096, 4096};
  process (operation, NULL);
  if (self->buf)
    {
      GeglBuffer *buffer = GEGL_BUFFER (self->buf);
      gegl_rectangle_set (&result, buffer->x, buffer->y,
                              buffer->width, buffer->height);
    }
  return result;
}

#endif

