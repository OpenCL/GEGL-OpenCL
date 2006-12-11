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

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            nop
#define GEGL_CHANT_DESCRIPTION     "Passthrough operation. Used mostly for routing/place holder purposes internally in GEGL."
#define GEGL_CHANT_SELF            "nop.c"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  gboolean success = FALSE;
  GeglBuffer *buffer = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "input"));
  if (buffer)
    {
      g_object_ref (buffer);
      gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (buffer));
      success = TRUE;
    } 
  return success;
}

#endif
