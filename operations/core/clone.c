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

property_string (ref, _("Reference"), "ID")
    description (_("The reference ID used as input (for use in XML)."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     clone
#define GEGL_OP_C_SOURCE clone.c

#include "gegl-op.h"
#include <math.h>
#include <string.h>

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglRectangle have_rect = gegl_node_get_bounding_box (operation->node);

  if (x >= have_rect.x &&
      y >= have_rect.y &&
      x < have_rect.width &&
      y < have_rect.height)
    {
      return operation->node;
    }
  return NULL;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglBuffer *input;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a clone", output_prop);
      return FALSE;
    }

  input  = gegl_operation_context_get_source (context, "input");
  if (!input)
    {
      g_warning ("clone received NULL input");
      return FALSE;
    }

  gegl_operation_context_take_object (context, "output", G_OBJECT (input));
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->detect = detect;
  operation_class->no_cache = TRUE;

  gegl_operation_class_set_keys (operation_class,
       "name",          "gegl:clone",
       "title",         _("Clone"),
       "description",   _("Clone a buffer, this is the same as gegl:nop but can get special treatment to get more human readable references in serializations/UI."),
       "categories",    "core",
       NULL);
}

#endif
