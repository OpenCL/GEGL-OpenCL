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
 * Copyright 2007 Étienne Bersac <bersace03@laposte.net>
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2013 Daniel Sabo
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_format (format, _("Output format"), NULL)
  description(_("The babl format of the output"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     convert_format
#define GEGL_OP_C_SOURCE convert-format.c

#include "gegl-op.h"

static void
prepare (GeglOperation *self)
{
  GeglProperties *o = GEGL_PROPERTIES (self);

  if (o->format)
    gegl_operation_set_format (self, "output", o->format);
  else
    gegl_operation_set_format (self, "output", gegl_operation_get_source_format (self, "input"));
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *roi,
         gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer *input;
  GeglBuffer *output;

  input  = gegl_operation_context_get_source (context, "input");

  if (gegl_buffer_get_format (input) != o->format)
    {
      output = gegl_operation_context_get_target (context, "output");
      gegl_buffer_copy (input, roi, GEGL_ABYSS_NONE,
                        output, roi);
      g_object_unref (input);
    }
  else
    {
      gegl_operation_context_take_object (context, "output", G_OBJECT (input));
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare  = prepare;
  operation_class->process  = process;
  operation_class->no_cache = FALSE;

  gegl_operation_class_set_keys (operation_class,
                "name",       "gegl:convert-format",
                "title",      _("Convert Format"),
                "categories", "core:color",
                "description", _("Convert the data to the specified format"),
                NULL);
}

#endif
