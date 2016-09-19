
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
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description    (_("Target path and filename, use '-' for stdout."))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME rgbe_save
#define GEGL_OP_C_SOURCE rgbe-save.c

#include "gegl-op.h"
#include "rgbe/rgbe.h"


static const gchar *FORMAT = "RGB float";


static gboolean
gegl_rgbe_save_process (GeglOperation       *operation,
                        GeglBuffer          *input,
                        const GeglRectangle *rect,
                        gint                 level)
{
  GeglProperties *o       = GEGL_PROPERTIES (operation);
  gfloat     *pixels  = NULL;
  gboolean    success = FALSE;

  /* Write the scanlines */
  pixels = g_malloc (rect->width        *
                     rect->height       *
                     sizeof (pixels[0]) *
                     babl_format_get_n_components (babl_format (FORMAT)));

  gegl_buffer_get (input, rect, 1.0, babl_format (FORMAT), pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (!rgbe_save_path (o->path, rect->width, rect->height, pixels))
      goto cleanup;
  success = TRUE;

cleanup:
  g_free (pixels);
  return success;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = gegl_rgbe_save_process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:rgbe-save",
    "title",        _("RGBE File Saver"),
    "categories" ,  "output",
    "description" ,
        _("RGBE image saver (Radiance HDR format)"),
    NULL);

  gegl_operation_handlers_register_saver (
    ".hdr", "gegl:rgbe-save");
  gegl_operation_handlers_register_saver (
    ".pic", "gegl:rgbe-save");
}

#endif

