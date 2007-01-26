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
gegl_chant_object (buffer, "GeglBuffer to write into")
#else

#define GEGL_CHANT_SINK
#define GEGL_CHANT_NAME            save_buffer
#define GEGL_CHANT_DESCRIPTION     "A GEGL buffer destination surface."
#define GEGL_CHANT_SELF            "save-buffer.c"
#define GEGL_CHANT_CATEGORIES      "programming:output"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

static void
dispose (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);

  if (self->buffer)
    {
      g_object_unref (self->buffer);
      self->buffer = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->dispose (object);
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer          *input;

  if (self->buffer)
    {
      /*FIXME: iterate the buffer in chunks/tiles/spans instead of making a large
       * temporary linear buffer the size of the incoming buffe
       */
      gpointer       format;
      guchar        *temp;
      GeglRectangle  rect;
      input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
      g_assert (input);

      gegl_rect_set (&rect, input->x, input->y, input->width, input->height);

      temp = g_malloc (gegl_buffer_pixels (input) * gegl_buffer_px_size (input));
      format = input->format;
      gegl_buffer_get (input, &rect, 1.0, format, temp);

      gegl_buffer_set (GEGL_BUFFER (self->buffer), &rect, format, temp);
      g_free (temp);
    }
  return TRUE;
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
