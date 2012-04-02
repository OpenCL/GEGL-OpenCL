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

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_color (value, _("Color"), "black",
                  _("The color to render (defaults to 'black')"))
#else

#define GEGL_CHANT_TYPE_POINT_RENDER
#define GEGL_CHANT_C_FILE           "color.c"

#include "gegl-chant.h"

static void
gegl_color_op_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
gegl_color_op_get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
gegl_color_op_process (GeglOperation       *operation,
                       void                *out_buf,
                       glong                n_pixels,
                       const GeglRectangle *roi,
                       gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *out_pixel = out_buf;
  gfloat      color[4];

  gegl_color_get_pixel (o->value, babl_format ("RGBA float"), color);

  while (n_pixels--)
    {
      out_pixel[0]=color[0];
      out_pixel[1]=color[1];
      out_pixel[2]=color[2];
      out_pixel[3]=color[3];

      out_pixel += 4;
    }
  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  point_render_class->process       = gegl_color_op_process;
  operation_class->get_bounding_box = gegl_color_op_get_bounding_box;
  operation_class->prepare          = gegl_color_op_prepare;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:color",
    "categories" , "render",
    "description",
      _("Generates a buffer entirely filled with the specified color, "
        "crop it to get smaller dimensions."),
    NULL);
}

#endif
