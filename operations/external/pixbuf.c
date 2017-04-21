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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_object (pixbuf, _("Pixbuf"), GDK_TYPE_PIXBUF)
    description(_("GdkPixbuf to use"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     pixbuf
#define GEGL_OP_C_SOURCE pixbuf.c

#include "gegl-op.h"
#include <gdk-pixbuf/gdk-pixdata.h>

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};

  if (!o->pixbuf)
    {
      return result;
    }

  result.x = 0;
  result.y = 0;
  result.width  = gdk_pixbuf_get_width (GDK_PIXBUF (o->pixbuf));
  result.height = gdk_pixbuf_get_height (GDK_PIXBUF (o->pixbuf));
  return result;
}

static void prepare (GeglOperation *operation)
{
  const Babl *format;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gboolean has_alpha;

  has_alpha = gdk_pixbuf_get_has_alpha (GDK_PIXBUF (o->pixbuf));
  format = has_alpha ? babl_format ("R'G'B'A u8") : babl_format ("R'G'B' u8");
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->pixbuf)
    {
      GeglRectangle extent;
      gint stride;

      stride = gdk_pixbuf_get_rowstride (GDK_PIXBUF (o->pixbuf));

      extent.x = 0;
      extent.y = 0;
      extent.width = gdk_pixbuf_get_width (GDK_PIXBUF (o->pixbuf));
      extent.height = gdk_pixbuf_get_height (GDK_PIXBUF (o->pixbuf));

      gegl_buffer_set (output, &extent, 0, NULL, gdk_pixbuf_read_pixels (GDK_PIXBUF (o->pixbuf)),
                       stride);
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  /*operation_class->no_cache = TRUE;*/

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:pixbuf",
    "title",        _("GdkPixbuf Source"),
    "categories"  , "programming:input",
    "description" ,
       _("Uses the GdkPixbuf located at the memory location in <em>pixbuf</em>."),
       NULL);
}

#endif

