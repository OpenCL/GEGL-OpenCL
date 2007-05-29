/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_string (ref, "", "The reference ID used as input.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME        clone
#define GEGL_CHANT_DESCRIPTION "A routing op used by the XML data model to implement clones."

#define GEGL_CHANT_SELF        "clone.c"
#define GEGL_CHANT_CATEGORIES  "misc"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"


/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  gboolean success = FALSE;
  GeglBuffer *buffer = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  if (buffer)
    {
      g_object_ref (buffer);
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (buffer));
      success = TRUE;
    } 
  return success;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *node = operation->node;

  if (x>=node->have_rect.x &&
      y>=node->have_rect.y &&
      x<node->have_rect.width  &&
      y<node->have_rect.height )
    {
      return node;
    }
  return NULL;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->detect = detect;
  operation_class->no_cache = TRUE;
}

#endif
