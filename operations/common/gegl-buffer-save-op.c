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
 * Copyright 2010 Martin Nordholts <martinn@src.gnome.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "/tmp/gegl-buffer.gegl")
  description (_("Target file path to write GeglBuffer to."))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME     gegl_buffer_save_op
#define GEGL_OP_C_SOURCE gegl-buffer-save-op.c

#include "gegl-op.h"


static gboolean
gegl_buffer_save_op_process (GeglOperation       *operation,
                             GeglBuffer          *input,
                             const GeglRectangle *result,
                             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_buffer_save (input, o->path, result);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process    = gegl_buffer_save_op_process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:gegl-buffer-save",
    "categories" , "hidden",
    "description", _("GeglBuffer file writer."),
    NULL);

  gegl_operation_handlers_register_saver (
    ".gegl", "gegl:gegl-buffer-save");
}

#endif
