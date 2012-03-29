
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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "rgbe-load.c"

#include "gegl-chant.h"

#include "rgbe/rgbe.h"

#include <errno.h>
#include <stdio.h>


static const gchar* FORMAT = "RGBA float";


static GeglRectangle
gegl_rgbe_load_get_bounding_box (GeglOperation *operation)
{
  GeglChantO       *o        = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle     result   = {0,0,0,0};
  rgbe_file        *file;
  guint             width, height;

  gegl_operation_set_format (operation,
                             "output",
                             babl_format (FORMAT));

  file = rgbe_load_path (o->path);
  if (!file)
      goto cleanup;

  if (!rgbe_get_size (file, &width, &height))
      goto cleanup;

  result.width  = width;
  result.height = height;

cleanup:
  rgbe_file_free (file);
  return result;
}


static gboolean
gegl_rgbe_load_process (GeglOperation       *operation,
                        GeglBuffer          *output,
                        const GeglRectangle *result,
                        gint                 level)
{
  GeglChantO       *o       = GEGL_CHANT_PROPERTIES (operation);
  gboolean          success = FALSE;
  gfloat           *pixels  = NULL;
  rgbe_file        *file;
  guint             width, height;

  file = rgbe_load_path (o->path);
  if (!file)
      goto cleanup;

  if (!rgbe_get_size (file, &width, &height))
      goto cleanup;

  if (width  != result->width  ||
      height != result->height)
      goto cleanup;

  pixels = rgbe_read_scanlines (file);
  if (!pixels)
    goto cleanup;

  gegl_buffer_set (output, result, 0, babl_format (FORMAT), pixels,
                   GEGL_AUTO_ROWSTRIDE);
  success = TRUE;

cleanup:
  g_free         (pixels);
  rgbe_file_free (file);

  return success;
}


static GeglRectangle
gegl_rgbe_load_get_cached_region (GeglOperation *operation,
                                  const GeglRectangle *roi)
{
  return gegl_rgbe_load_get_bounding_box (operation);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = gegl_rgbe_load_process;
  operation_class->get_bounding_box  = gegl_rgbe_load_get_bounding_box;
  operation_class->get_cached_region = gegl_rgbe_load_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:rgbe-load",
    "categories"  , "hidden",
    "description" , _("RGBE image loader (Radiance HDR format)."),
    NULL);

  gegl_extension_handler_register (".hdr", "gegl:rgbe-load");
  gegl_extension_handler_register (".pic", "gegl:rgbe-load");
}

#endif

