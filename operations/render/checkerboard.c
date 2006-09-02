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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (x,        -G_MAXINT, G_MAXINT, 16, "")
gegl_chant_int (y,        -G_MAXINT, G_MAXINT, 16, "")
gegl_chant_int (x_offset, -G_MAXINT, G_MAXINT,  0, "")
gegl_chant_int (y_offset, -G_MAXINT, G_MAXINT,  0, "")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           checkerboard
#define GEGL_CHANT_DESCRIPTION    "Black and white checkerboard renderer."

#define GEGL_CHANT_SELF           "checkerboard.c"
#define GEGL_CHANT_CATEGORIES      "sources:render"
#include "gegl-chant.h"

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglRect  *need;
  GeglOperationSource  *op_source = GEGL_OPERATION_SOURCE(operation);
  ChantInstance *self = GEGL_CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  need = gegl_operation_need_rect (operation);
  {
    GeglRect *result = gegl_operation_result_rect (operation);
    gfloat *buf;
    gfloat color1[4] = {1., 1., 1., 1.};
    gfloat color2[4] = {0., 0., 0., 1.};

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
        gint y;
        for (y=0; y < result->h; y++)
          {
            gint x;
            for (x=0; x < result->w; x++)
              {
                gfloat *rgba_color;
                gint nx,ny;

                nx = (x + result->x + self->x_offset)/self->x;
                ny = (y + result->y + self->y_offset)/self->y;

                rgba_color = (nx+ny)%2 == 0 ? color1 : color2;

                memcpy(dst, rgba_color, 4*sizeof(gfloat));
                dst += 4;
              }
          }
      }
    gegl_buffer_set (op_source->output, buf);
    g_free (buf);
  }
  return  TRUE;
}


static gboolean
calc_have_rect (GeglOperation *operation)
{
  /*OpNoise    *self      = (OpNoise*)(operation);*/

  gegl_operation_set_have_rect (operation, -1000000, -1000000, 2000000, 2000000);

  return TRUE;
}


#endif
