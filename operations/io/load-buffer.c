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
#if GEGL_CHANT_PROPERTIES
gegl_chant_object (buffer, "GeglBuffer to use")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME         load_buffer
#define GEGL_CHANT_DESCRIPTION  "A source that uses an in-memory GeglBuffer, for use internally by GEGL."

#define GEGL_CHANT_SELF         "load-buffer.c"
#define GEGL_CHANT_CATEGORIES   "programming:input"
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
         GeglNodeContext *context,
         const GeglRectangle *result)
{
  GeglChantOperation       *self = GEGL_CHANT_OPERATION (operation);
  if (self->buffer)
    {
      g_object_ref (self->buffer); /* Add an extra reference, since gegl_operation_set_data
                                      is stealing one.
                                    */
      gegl_node_context_set_object (context, "output", G_OBJECT (self->buffer));
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
  result = *gegl_buffer_get_extent (GEGL_BUFFER (self->buffer));
  return result;
}

static void class_init (GeglOperationClass *operation_class)
{
  G_OBJECT_CLASS (operation_class)->dispose = dispose;
  operation_class->no_cache = TRUE;
}

#endif
