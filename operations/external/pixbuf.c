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

gegl_chant_pointer (pixbuf, _("Pixbuf"), _("GdkPixbuf to use"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "pixbuf.c"

#include "gegl-chant.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};

  if (!o->pixbuf)
    {
      return result;
    }

  result.x = 0;
  result.y = 0;
  result.width  = gdk_pixbuf_get_width (o->pixbuf);
  result.height = gdk_pixbuf_get_height (o->pixbuf);
  return result;
}

static void prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gegl_operation_set_format (operation, "output", 
      babl_format(gdk_pixbuf_get_has_alpha(o->pixbuf)?"R'G'B'A u8":"R'G'B' u8"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->pixbuf)
    {
      GeglRectangle extent;

      extent.x = 0;
      extent.y = 0;
      extent.width = gdk_pixbuf_get_width (o->pixbuf);
      extent.height = gdk_pixbuf_get_height (o->pixbuf);

      gegl_buffer_set (output, &extent, NULL, gdk_pixbuf_get_pixels (o->pixbuf),
                       GEGL_AUTO_ROWSTRIDE);
    }
  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  /*operation_class->no_cache = TRUE;*/

  operation_class->name        = "gegl:pixbuf";
  operation_class->categories  = "programming:input";
  operation_class->description =
        _("Uses the GdkPixbuf located at the memory location in <em>pixbuf</em>.");
}

#endif

