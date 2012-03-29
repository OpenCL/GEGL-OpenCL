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
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_object(buffer, _("Input buffer"),
		  _("The GeglBuffer to load into the pipeline"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "buffer-source.c"

#include "gegl-chant.h"

static void buffer_changed (GeglBuffer          *buffer,
                            const GeglRectangle *rect,
                            gpointer             userdata)
{
  gegl_operation_invalidate (GEGL_OPERATION (userdata), rect, FALSE);
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);

  if (!o->buffer)
    {
      return result;
    }
  result = *gegl_buffer_get_extent (GEGL_BUFFER (o->buffer));
  return result;
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_pad,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->buffer)
    {
      if (!o->chant_data)
        {
          o->chant_data = (void*)0xf00;
          g_signal_connect (o->buffer, "changed",
                            G_CALLBACK(buffer_changed), operation);
        }
      g_object_ref (o->buffer); /* Add an extra reference, since
				     * gegl_operation_set_data is
				     * stealing one.
				     */

      /* override core behaviour, by resetting the buffer in the operation_context */
      gegl_operation_context_take_object (context, "output",
                                          G_OBJECT (o->buffer));
    }
  return TRUE;
}

static void
dispose (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->buffer)
    {
      g_object_unref (o->buffer);
      o->buffer = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->dispose (object);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  G_OBJECT_CLASS (klass)->dispose = dispose;

  gegl_operation_class_set_keys (operation_class,
      "name",       "gegl:buffer-source",
      "categories", "programming:input",
      "description", _("A source that uses an in-memory GeglBuffer, for use internally by GEGL."),
      NULL);

  operation_class->no_cache = TRUE;
}

#endif
