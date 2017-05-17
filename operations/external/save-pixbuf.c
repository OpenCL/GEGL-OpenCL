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
 * Copyright 2007 Étienne Bersac <bersace03@laposte.net>
 */

#include "config.h"

#include <babl/babl.h>

#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_pointer (pixbuf, _("Pixbuf location"),
                    _("The location where to store the output GdkPixbuf."))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME save_pixbuf
#define GEGL_OP_C_SOURCE save-pixbuf.c

#include "gegl-op.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->pixbuf)
    {
      GdkPixbuf       **pixbuf = o->pixbuf;
      const Babl       *babl;
      const Babl       *format;
      guchar           *temp;
      GeglRectangle *rect = gegl_operation_source_get_bounding_box (operation, "input");
      gchar *name;
      gboolean has_alpha;
      gint bpp;
      gint bps;
      gint stride;

      g_object_get (input, "format", &format, NULL);

      has_alpha = babl_format_has_alpha (format);

      /* pixbuf from data only support 8bit bps */
      bps = 8;
      name = g_strdup_printf ("R'G'B'%s u%i",
                  has_alpha ? "A" : "",
                  bps);
      babl = babl_format (name);

      bpp = babl_format_get_bytes_per_pixel (babl);
      stride = bpp * rect->width;

      temp = g_malloc0_n (stride, rect->height);
      gegl_buffer_get (input, rect, 1.0, babl, temp, stride,
                       GEGL_ABYSS_NONE);
      if (temp) {
    *pixbuf = gdk_pixbuf_new_from_data (temp,
                        GDK_COLORSPACE_RGB,
                        has_alpha,
                        bps,
                        rect->width, rect->height,
                        stride,
                        (GdkPixbufDestroyNotify) g_free, NULL);
      }
      else {
    g_warning (G_STRLOC ": inexistant data, unable to create GdkPixbuf.");
      }

      g_free (name);
    }
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:save-pixbuf",
    "title",        _("Store in GdkPixbuf"),
    "categories"  , "programming:output",
    "description" , _("Store image in a GdkPixbuf."),
    NULL);

}

#endif
