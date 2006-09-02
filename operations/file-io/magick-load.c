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
#ifdef CHANT_SELF
chant_string (path, "/tmp/gegl-logo.svg", "path to file to load")
chant_pointer (cached, "private")
#else

#define CHANT_SOURCE
#define CHANT_NAME            magick_load
#define CHANT_DESCRIPTION     "Image Magick wrapper, that converts to" \
                              "PNG before loading."

#define CHANT_SELF            "magick-load.c"
#define CHANT_CLASS_CONSTRUCT
#define CHANT_CATEGORIES      "hidden"
#include "gegl-chant.h"
#include <stdio.h>

static void
load_cache (ChantInstance *op_magick_load)
{
  if (!op_magick_load->cached)
    {
        GeglNode *temp_gegl;
        gchar xml[1024]="";
          {
            /* ImageMagick backed fallback FIXME: make this robust.
             * maybe use pipes in a manner similar to the raw loader */
            gchar cmd_buf[1024];
            sprintf (cmd_buf, "convert \"%s\" /tmp/gegl-magick.png", op_magick_load->path);
            system (cmd_buf);

            sprintf (xml, "<gegl><tree><node operation='png-load' path='/tmp/gegl-magick.png'></node></tree></gegl>");
          }

    temp_gegl = gegl_xml_parse (xml);
    gegl_node_apply (temp_gegl, "output");

    /*FIXME: this should be unneccesary, using the graph
     * directly as a node is more elegant.
     */
    gegl_node_get (temp_gegl, "output", &(op_magick_load->cached), NULL);
    g_object_unref (temp_gegl);
  }
}

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  ChantInstance       *self = CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  g_assert (self->cached);
  {
    GeglRect *result = gegl_operation_result_rect (operation);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "source", self->cached,
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
    g_object_unref (self->cached); /* do not keep a cache here, the meta loader has one */
    self->cached = NULL;
  }
  return  TRUE;
}


static gboolean
calc_have_rect (GeglOperation *operation)
{
  ChantInstance *self = CHANT_INSTANCE (operation);
  gint width, height;

  load_cache (self);

  width  = GEGL_BUFFER (self->cached)->width;
  height = GEGL_BUFFER (self->cached)->height;

  gegl_operation_set_have_rect (operation, 0, 0, width, height);

  return TRUE;
}

static void dispose (GObject *gobject)
{
  ChantInstance *self = CHANT_INSTANCE (gobject);
  if (self->cached)
    {
      g_object_unref (self->cached);
      self->cached = NULL;
    }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (gobject)))->dispose (gobject);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
