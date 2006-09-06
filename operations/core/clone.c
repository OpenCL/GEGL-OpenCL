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
#include "gegl-chant.h"


/* Actual image processing code
 ************************************************************************/
static gboolean
process (GeglOperation *operation,
         const gchar   *output_prop)
{
  GeglOperationFilter   *filter = GEGL_OPERATION_FILTER(operation);
  
  if (filter->input)
    filter->output = g_object_ref (filter->input);
  return  TRUE;
}

#endif
