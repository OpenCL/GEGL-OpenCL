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

gegl_chant_color (value, "black", "One of the cell colors (defaults to 'black')")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           color
#define GEGL_CHANT_DESCRIPTION    "Generates a buffer entirely filled with the specified color, crop it to get smaller dimensions."

#define GEGL_CHANT_SELF           "color.c"
#define GEGL_CHANT_CATEGORIES     "render"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglRectangle      *need;
  GeglBuffer         *output = NULL;
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  need = gegl_operation_get_requested_region (operation, context_id);
  {
    GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    gfloat *buf;
    gfloat color[4];

    gegl_color_get_rgba (self->value,
                         &color[0],
                         &color[1],
                         &color[2],
                         &color[3]);

    output = g_object_new (GEGL_TYPE_BUFFER,
                           "format",
                           babl_format ("RGBA float"),
                           "x",      result->x,
                           "y",      result->y,
                           "width",  result->w,
                           "height", result->h,
                           NULL);
    buf = g_malloc (gegl_buffer_pixels (output) * gegl_buffer_px_size (output));
      {
        gfloat *dst=buf;
        gint i;
        for (i=0; i < result->h*result->w; i++)
          {
            memcpy(dst, color, 4*sizeof(gfloat));
            dst += 4;
          }
      }
    gegl_buffer_set (output, NULL, buf, NULL);
    g_free (buf);
  }
  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
  return  TRUE;
}


static GeglRectangle 
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-10000000,-10000000,20000000,20000000};
  return result;
}

#endif
