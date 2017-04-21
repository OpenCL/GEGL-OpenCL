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
  description(_("Path of GeglBuffer file to load."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     gegl_buffer_load_op
#define GEGL_OP_C_SOURCE gegl-buffer-load-op.c

#include "gegl-op.h"


static void
gegl_buffer_load_op_ensure_buffer (GeglProperties *o)
{
  if (!o->user_data)
    o->user_data = gegl_buffer_load (o->path);
}

static GeglRectangle
gegl_buffer_load_op_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglProperties    *o      = GEGL_PROPERTIES (operation);

  gegl_buffer_load_op_ensure_buffer (o);

  if (o->user_data)
    {
      result.width  = gegl_buffer_get_width  (GEGL_BUFFER (o->user_data));
      result.height = gegl_buffer_get_height (GEGL_BUFFER (o->user_data));
    }

  return result;
}

static gboolean
gegl_buffer_load_op_process (GeglOperation        *operation,
                             GeglOperationContext *context,
                             const gchar          *output_pad,
                             const GeglRectangle  *result,
                             gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_buffer_load_op_ensure_buffer (o);

  gegl_operation_context_take_object (context, output_pad, o->user_data);
  o->user_data = NULL;

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process          = gegl_buffer_load_op_process;
  operation_class->get_bounding_box = gegl_buffer_load_op_get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:gegl-buffer-load",
    "categories" , "hidden",
    "description", _("GeglBuffer file loader."),
    NULL);

  gegl_operation_handlers_register_loader (
    ".gegl", "gegl:gegl-buffer-load");
}

#endif
