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

gegl_chant_color (value, "black", "The color to render (defaults to 'black')")
#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME           color
#define GEGL_CHANT_DESCRIPTION    "Generates a buffer entirely filled with the specified color, crop it to get smaller dimensions."

#define GEGL_CHANT_SELF           "color.c"
#define GEGL_CHANT_CATEGORIES     "render"

#define GEGL_CHANT_CLASS_INIT
#define GEGL_CHANT_PREPARE

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation,
         gpointer       context_id)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  const GeglRectangle *need;
  GeglBuffer          *output = NULL;

  need = gegl_operation_get_requested_region (operation, context_id);
  {
    const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    gfloat              *buf;
    gfloat               color[4];

    gegl_color_get_rgba (self->value,
                         &color[0],
                         &color[1],
                         &color[2],
                         &color[3]);

    output = gegl_operation_get_target (operation, context_id, "output");
    buf = g_malloc (result->width * result->height * 4 * sizeof (gfloat));
      {
        gfloat *dst=buf;
        gint i;
        for (i=0; i < result->height *result->width ; i++)
          {
            memcpy(dst, color, 4*sizeof(gfloat));
            dst += 4;
          }
      }
    gegl_buffer_set (output, NULL, NULL, buf);
    g_free (buf);
  }
  return  TRUE;
}


static GeglRectangle 
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {-10000000,-10000000,20000000,20000000};
  return result;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  return NULL;
}

static void class_init (GeglOperationClass *klass)
{
  klass->detect = detect;
  klass->no_cache = TRUE;
  klass->adjust_result_region = NULL;
}

#endif
