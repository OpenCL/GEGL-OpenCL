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

gegl_chant_string (ref, _("Reference"), "ID",
                   _("The reference ID used as input (for use in XML)."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "clone.c"

#include "gegl-chant.h"
#include "graph/gegl-node.h"
#include <math.h>
#include <string.h>

#include "buffer/gegl-buffer-cl-cache.h"

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *node = operation->node;

  if (x >= node->have_rect.x &&
      y >= node->have_rect.y &&
      x < node->have_rect.width &&
      y < node->have_rect.height)
    {
      return node;
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
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->detect = detect;
  operation_class->no_cache = TRUE;

  gegl_operation_class_set_keys (operation_class,
       "name",          "gegl:clone",
       "description",   _("Clone a buffer"),
       "categories",    "core",
       NULL);
}

#endif
