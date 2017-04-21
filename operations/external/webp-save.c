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
  description (_("Target path and filename, use '-' for stdout"))

property_int (quality, _("Quality"), 90)
  description (_("WebP compression quality"))
  value_range (1, 100)

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME webp_save
#define GEGL_OP_C_SOURCE webp-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <webp/encode.h>

static int
write_to_stream (const uint8_t*     data,
                 size_t             size,
                 const WebPPicture* picture)
{
  GOutputStream *stream = (GOutputStream *) picture->custom_ptr;
  GError *error = NULL;
  gboolean success;
  gsize written;

  g_assert (stream);

  success = g_output_stream_write_all (G_OUTPUT_STREAM (stream),
                                       (const void *) data, size, &written,
                                       NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return 0;
    }

  return 1;
}

static gint
save_RGBA (WebPPicture         *picture,
           GeglBuffer          *input,
           const GeglRectangle *result,
           const Babl          *format)
{
  gint bytes_per_pixel, bytes_per_row;
  uint8_t *buffer;

  bytes_per_pixel = babl_format_get_bytes_per_pixel (format);
  bytes_per_row = bytes_per_pixel * result->width;

  buffer = g_try_new (uint8_t, bytes_per_row * result->height);

  g_assert (buffer != NULL);

  gegl_buffer_get (input, result, 1.0, format, buffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  WebPPictureImportRGBA (picture, buffer, bytes_per_row);

  g_free (buffer);
  return 0;
}

static gboolean
export_webp (GeglOperation       *operation,
             GeglBuffer          *input,
             const GeglRectangle *result,
             GOutputStream       *stream)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  WebPConfig config;
  WebPPicture picture;
  const Babl *format;
  gint status;

  g_return_val_if_fail (stream != NULL, FALSE);

  if (!WebPConfigInit (&config) || !WebPPictureInit (&picture))
    {
      g_warning ("could not initialize WebP encoder");
      return FALSE;
    }

  if (!WebPConfigPreset (&config, WEBP_PRESET_DEFAULT, o->quality))
    {
      g_warning("could not load WebP default preset");
      return FALSE;
    }

  picture.width = result->width;
  picture.height = result->height;

  format = babl_format ("R'G'B'A u8");

  if (!WebPValidateConfig (&config))
    {
      g_warning ("WebP encoder configuration is invalid");
      return FALSE;
    }

  picture.writer = write_to_stream;
  picture.custom_ptr = stream;

  if (save_RGBA (&picture, input, result, format))
    {
      g_warning ("could not pass pixels data to WebP encoder");
      return FALSE;
    }

  status = WebPEncode (&config, &picture);

  WebPPictureFree (&picture);

  return status ? TRUE : FALSE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GOutputStream *stream;
  GFile *file = NULL;
  GError *error = NULL;
  gboolean status = TRUE;

  stream = gegl_gio_open_output_stream (NULL, o->path, &file, &error);
  if (stream == NULL)
    {
      status = FALSE;
      g_warning ("%s", error->message);
      goto cleanup;
    }

  if (!export_webp (operation, input, result, stream))
    {
      status = FALSE;
      g_warning ("could not export WebP file");
      goto cleanup;
    }

cleanup:
  if (stream != NULL)
    g_object_unref (stream);

  if (file != NULL)
    g_object_unref (file);

  return status;
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
    "name",          "gegl:webp-save",
    "title",       _("WebP File Saver"),
    "categories",    "output",
    "description", _("WebP image saver"),
     NULL);

  gegl_operation_handlers_register_saver (
    ".webp", "gegl:webp-save");
}

#endif
