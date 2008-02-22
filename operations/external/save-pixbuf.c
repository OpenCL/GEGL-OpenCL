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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_pointer (pixbuf, "Pixbuf location",
                    "The location where to store the output GdkPixuf.")

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "save-pixbuf.c"

#include "gegl-chant.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->pixbuf)
    {
      GdkPixbuf       **pixbuf = o->pixbuf;
      const Babl       *babl;
      const BablFormat *format;
      guchar           *temp;
      GeglRectangle *rect = gegl_operation_source_get_bounding_box (operation, "input");
      gchar *name;
      gboolean has_alpha;
      gint bps;
      guint i;

      g_object_get (input, "format", &babl, NULL);
      format = (BablFormat*) babl;

      has_alpha = FALSE;
      for (i = 0; i < format->components; i++) {
    has_alpha = has_alpha || format->component[i]->alpha != 0;
      }

      /* pixbuf from data only support 8bit bps */
      bps = 8;
      name = g_strdup_printf ("RGB%s u%i",
                  has_alpha ? "A" : "",
                  bps);
      babl = babl_format (name);

      temp = g_malloc (rect->width * rect->height * bps);
      gegl_buffer_get (input, 1.0, rect, babl, temp, GEGL_AUTO_ROWSTRIDE);
      if (temp) {
    *pixbuf = gdk_pixbuf_new_from_data (temp,
                        GDK_COLORSPACE_RGB,
                        has_alpha,
                        bps,
                        rect->width, rect->height,
                        rect->width * (has_alpha ? 4 : 3) * bps/8,
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
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "save-pixbuf";
  operation_class->categories  = "programming:output";
  operation_class->description = "Save output into a GdkPixbuf.";

}

#endif
