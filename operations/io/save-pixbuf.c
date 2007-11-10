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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Étienne Bersac <bersace03@laposte.net>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_pointer (pixbuf, "The location where to store the output GdkPixuf.")

#else

#define GEGL_CHANT_SINK
#define GEGL_CHANT_NAME            save_pixbuf
#define GEGL_CHANT_DESCRIPTION     "Save output into a GdkPixbuf."
#define GEGL_CHANT_SELF            "save-pixbuf.c"
#define GEGL_CHANT_CATEGORIES      "programming:output"
#include "gegl-chant.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer          *input;

  if (self->pixbuf)
    {
      GdkPixbuf		**pixbuf = self->pixbuf;
      Babl          	*babl;
      BablFormat    	*format;
      guchar        	*temp;
      GeglRectangle	*rect = gegl_operation_source_get_defined_region (operation, "input");
      gchar *name;
      gboolean has_alpha;
      gint bps;
      guint i;

      input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
      g_assert (input);

      babl = input->format;
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

#endif
