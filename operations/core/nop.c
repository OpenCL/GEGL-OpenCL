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

   /* no properties */

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "nop.c"

#include "gegl-chant.h"


static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result)
{
  GeglBuffer *input;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a nop", output_prop);
      return FALSE;
    }

  input = GEGL_BUFFER (gegl_operation_context_get_object (context, "input"));
  if (!input)
    {
      g_warning ("nop received NULL input");
      return FALSE;
    }

  gegl_operation_context_take_object (context, "output", g_object_ref (G_OBJECT (input)));
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->process = process;

  operation_class->name       = "gegl:nop";
  operation_class->categories = "core";
  operation_class->description = _("No operation (can be used as a routing point)");
}

#endif
