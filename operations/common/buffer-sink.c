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

gegl_chant_pointer (buffer, _("Buffer location"),
                    _("The location where to store the output GeglBuffer"))
gegl_chant_pointer (format, _("babl format"),
                    _("The babl format of the output GeglBuffer, NULL to use input buffer format"))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "buffer-sink.c"

#include "gegl-chant.h"

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->buffer != NULL &&
      (o->format == NULL || o->format == gegl_buffer_get_format (input)))
    {
      GeglBuffer **output = o->buffer;

      *output = gegl_buffer_create_sub_buffer (input, result);
    }
  else if (o->buffer != NULL &&
           o->format != NULL)
    {
      GeglBuffer **output = o->buffer;

      *output = gegl_buffer_new (gegl_buffer_get_extent (input),
                                 o->format);

      gegl_buffer_copy (input, NULL,
                        *output, NULL);
    }

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name",       "gegl:buffer-sink",
      "categories", "programming:output",
      "description", _("A GEGL buffer destination surface."),
      NULL);
}

#endif
