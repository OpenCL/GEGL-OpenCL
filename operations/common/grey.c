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


#ifdef GEGL_PROPERTIES

   /* no properties */

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     grey
#define GEGL_OP_C_SOURCE grey.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("YA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/* XXX: could be sped up by special casing op-filter behavior */

static gboolean
process (GeglOperation        *op,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GObject *input = gegl_operation_context_get_object (context, "input");

  gegl_operation_context_take_object (context, "output", g_object_ref (input));
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = process;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:gray",
      "compat-name", "gegl:grey",
      "title",       _("Make Grey"),
      "categories" , "grayscale:color",
      "description", _("Turns the image grayscale"),
      NULL);
}

#endif
