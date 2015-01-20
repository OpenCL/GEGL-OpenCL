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
 * Copyright 2013 Michael Henning <drawoc@darkrefraction.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Target file path."))

property_int (quality, _("Quality"), 90)
  description (_("WebP compression quality"))
  value_range (1, 100)

#else

#define GEGL_OP_SINK
#define GEGL_OP_C_SOURCE webp-save.c

#include "gegl-op.h"
#include <webp/encode.h>
#include <stdio.h>

static int
write_func (const uint8_t* data, size_t data_size, const WebPPicture* const pic)
{
  FILE* const out = (FILE*) pic->custom_ptr;

  if (!data_size) return 1;
  return fwrite (data, data_size, 1, out) == 1;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const GeglRectangle *bounds = gegl_buffer_get_extent (input);

  FILE* file;
  int status;

  WebPConfig config;
  WebPPicture pic;

  if (!WebPConfigPreset (&config, WEBP_PRESET_DEFAULT, o->quality))
    return FALSE;

  if (!WebPValidateConfig (&config))
    return FALSE;

  if (!WebPPictureInit (&pic))
    return FALSE;

  pic.width = bounds->width;
  pic.height = bounds->height;

  file = fopen (o->path, "wb");
  pic.writer = write_func;
  pic.custom_ptr = file;

  {
    gint rowstride = bounds->width * sizeof (char) * 4;
    gpointer temp = g_malloc (rowstride * bounds->height);

    gegl_buffer_get (input, bounds, 1.0, babl_format ("R'G'B'A u8"), temp,
                     rowstride, GEGL_ABYSS_NONE);

    WebPPictureImportRGBA (&pic, temp, rowstride);

    g_free (temp);
  }

  status = WebPEncode (&config, &pic);
  WebPPictureFree (&pic);
  fclose (file);

  return status ? TRUE : FALSE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process    = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
  "name",        "gegl:webp-save",
  "title",       _("WebP File Saver"),
  "categories" , "output",
  "description", _("WebP image saver."),
   NULL);

  gegl_extension_handler_register_saver (".webp", "gegl:webp-save");
}

#endif
