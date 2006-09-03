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

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           blank
#define GEGL_CHANT_DESCRIPTION    "Blank Source"

#define GEGL_CHANT_SELF           "blank.c"
#define GEGL_CHANT_CATEGORIES      "sources"
#include "gegl-chant.h"

static gboolean
process (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglRect  *need;
  GeglOperationSource     *op_source = GEGL_OPERATION_SOURCE(operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  need = gegl_operation_need_rect (operation);

  {
    GeglRect *result = gegl_operation_result_rect (operation);
    gfloat *buf;

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "format", 
                        babl_format ("YA float"),
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
    buf = g_malloc0 (gegl_buffer_pixels (op_source->output) * gegl_buffer_px_size (op_source->output));
    gegl_buffer_set (op_source->output, buf);
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
