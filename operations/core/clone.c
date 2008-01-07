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


/* FIXME: rewrtie without chanting, wihtout deriving from GeglOperationFilter, but
 *        directly from GeglOperation
 */

/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *input,
         GeglBuffer          *output, /* ignored */
         const GeglRectangle *result)
{
  gboolean success = FALSE;
  if (input)
    {
      g_object_ref (input);
      /* overrides the _set_object, that has occured earlier */
      gegl_node_context_set_object (context, "output", G_OBJECT (input));
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
