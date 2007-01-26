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
gegl_chant_pointer (pixbuf, "GdkPixbuf to use")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            pixbuf
#define GEGL_CHANT_DESCRIPTION     "Uses the GdkPixbuf located at the memory location in <em>pixbuf</em>."

#define GEGL_CHANT_SELF            "pixbuf.c"
#define GEGL_CHANT_CATEGORIES      "programming:input"
#include "gegl-chant.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self      = GEGL_CHANT_OPERATION (operation);

  if (self->pixbuf)
    {
      GeglBuffer *output = g_object_new (GEGL_TYPE_BUFFER,
                                        "format", babl_format(gdk_pixbuf_get_has_alpha(self->pixbuf)?"R'G'B'A u8":"R'G'B' u8"),
                                        "x", 0,
                                        "y", 0,
                                        "width", gdk_pixbuf_get_width (self->pixbuf),
                                        "height", gdk_pixbuf_get_height (self->pixbuf),
                                        NULL);
      gegl_buffer_set (output, NULL, NULL, gdk_pixbuf_get_pixels (self->pixbuf));
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
    }
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  if (!self->pixbuf)
    {
      return result;
    }
    
  result.x = 0;
  result.y = 0;
  result.w = gdk_pixbuf_get_width (self->pixbuf);
  result.h = gdk_pixbuf_get_height (self->pixbuf);
  return result;
}

#endif

