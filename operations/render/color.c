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
         gpointer       dynamic_id)
{
  GeglRect  *need;
  GeglOperationSource  *op_source = GEGL_OPERATION_SOURCE(operation);
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);

  need = gegl_operation_get_requested_region (operation, dynamic_id);
  {
    GeglRect *result = gegl_operation_result_rect (operation, dynamic_id);
    gfloat *buf;
    gfloat color[4];

    gegl_color_get_rgba (self->value,
                         &color[0],
                         &color[1],
                         &color[2],
                         &color[3]);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "format",
                        babl_format ("RGBA float"),
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
    buf = g_malloc (gegl_buffer_pixels (op_source->output) * gegl_buffer_px_size (op_source->output));
      {
        gfloat *dst=buf;
        gint i;
        for (i=0; i < result->h*result->w; i++)
          {
            memcpy(dst, color, 4*sizeof(gfloat));
            dst += 4;
          }
      }
    gegl_buffer_set (op_source->output, NULL, buf, NULL);
    g_free (buf);
  }
  return  TRUE;
}


static GeglRect 
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {-10000000,-10000000,20000000,20000000};
  return result;
}

#endif
