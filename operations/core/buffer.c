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
gegl_chant_object (buffer, "GeglBuffer to use")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            buffer
#define GEGL_CHANT_DESCRIPTION     "A source that uses an in-memory GeglBuffer, for use internally by GEGL."

#define GEGL_CHANT_SELF            "buffer.c"
#define GEGL_CHANT_CATEGORIES      "programming:input"
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
  GeglChantOperation       *self = GEGL_CHANT_OPERATION (operation);
  if (self->buffer)
    {
      g_object_ref (self->buffer); /* Add an extra reference, since gegl_operation_set_data
                                      is stealing one.
                                    */
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (self->buffer));
    }
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  if (!self->buffer)
    {
      return result;
    }
  result.x = GEGL_BUFFER (self->buffer)->x;
  result.y = GEGL_BUFFER (self->buffer)->y;
  result.w = GEGL_BUFFER (self->buffer)->width;
  result.h = GEGL_BUFFER (self->buffer)->height;
  return result;
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

#endif
