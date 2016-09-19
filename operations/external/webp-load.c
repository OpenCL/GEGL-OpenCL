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
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI for file to load"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME webp_load
#define GEGL_OP_C_SOURCE webp-load.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <webp/decode.h>

#define IO_BUFFER_SIZE 4096

typedef struct
{
  GFile *file;
  GInputStream *stream;

  WebPDecoderConfig *config;
  WebPIDecoder *decoder;

  const Babl *format;

  gint width;
  gint height;
} Priv;

static void
cleanup(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      if (p->decoder != NULL)
        WebPIDelete (p->decoder);
      p->decoder = NULL;
      if (p->config != NULL)
        WebPFreeDecBuffer (&p->config->output);
      if (p->config != NULL)
        g_free (p->config);
      p->config = NULL;

      if (p->stream != NULL)
        g_input_stream_close (G_INPUT_STREAM (p->stream), NULL, NULL);
      if (p->stream != NULL)
        g_clear_object (&p->stream);

      if (p->file != NULL)
        g_clear_object (&p->file);

      p->width = p->height = 0;
      p->format = NULL;
    }
}

static gsize
read_from_stream (GInputStream *stream,
                  guchar **buffer,
                  gsize size)
{
  GError *error = NULL;
  gboolean success;
  gsize read;

  *buffer = g_try_new (guchar, size);

  g_assert (*buffer != NULL);

  success = g_input_stream_read_all (G_INPUT_STREAM (stream),
                                     (void *) *buffer, size, &read,
                                     NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return -1;
    }

  return read;
}

static gsize
decode_from_stream (GInputStream *stream,
                    WebPIDecoder *decoder)
{
  GError *error = NULL;
  const gsize size = IO_BUFFER_SIZE;
  guchar *buffer;
  gsize read, total = 0;
  VP8StatusCode status;
  gboolean success;

  buffer = g_try_new (guchar, size);

  g_assert (buffer != NULL);

  do
    {
      success = g_input_stream_read_all (G_INPUT_STREAM (stream),
                                         (void *) buffer, size, &read,
                                         NULL, &error);
      if (!success || error != NULL)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
          return -1;
        }
      else if (read > 0)
        {
          total += read;

          status = WebPIAppend (decoder, buffer, read);
          if (status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
            return -1;
          else if (status == VP8_STATUS_OK)
            break;
        }
    }
  while (success && read > 0);

  return total;
}

static gboolean
query_webp (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;

  g_return_val_if_fail (p->config != NULL, FALSE);

  if (p->config->input.has_alpha)
    {
      p->config->output.colorspace = MODE_RGBA;
      p->format = babl_format ("R'G'B'A u8");
    }
  else
    {
      p->config->output.colorspace = MODE_RGB;
      p->format = babl_format ("R'G'B' u8");
    }

  p->height = p->config->input.height;
  p->width = p->config->input.width;

  return TRUE;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (o->user_data) ? o->user_data : g_new0 (Priv, 1);
  GError *error = NULL;
  GFile *file = NULL;
  guchar *buffer;
  gsize read;

  g_assert (p != NULL);

  if (p->file != NULL && (o->uri || o->path))
    {
      if (o->uri && strlen (o->uri) > 0)
        file = g_file_new_for_uri (o->uri);
      else if (o->path && strlen (o->path) > 0)
        file = g_file_new_for_path (o->path);
      if (file != NULL)
        {
          if (!g_file_equal (p->file, file))
            cleanup (operation);
          g_object_unref (file);
        }
    }

  o->user_data = (void*) p;

  if (p->config == NULL)
    {
      p->stream = gegl_gio_open_input_stream (o->uri, o->path, &p->file, &error);
      if (p->stream == NULL)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
          cleanup (operation);
          return;
        }

      p->config = g_try_new (WebPDecoderConfig, 1);
      p->decoder = WebPINewDecoder (&p->config->output);

      g_assert (p->config != NULL);

      if (!WebPInitDecoderConfig (p->config))
        {
          g_warning ("could not initialise WebP decoder configuration");
          cleanup (operation);
          return;
        }

      read = read_from_stream (p->stream, &buffer, IO_BUFFER_SIZE);
      if (WebPGetFeatures (buffer, read, &p->config->input) != VP8_STATUS_OK)
        {
          g_warning ("failed reading WebP image file");
          cleanup (operation);
          g_free (buffer);
          return;
        }

      if (!query_webp (operation))
        {
          g_warning ("could not query WebP image file");
          cleanup (operation);
          g_free (buffer);
          return;
        }

       WebPIAppend (p->decoder, buffer, read);

      g_free (buffer);
    }

  gegl_operation_set_format (operation, "output", p->format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  Priv *p = (Priv*) o->user_data;

  if (p->config != NULL)
    {
      result.width = p->width;
      result.height = p->height;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;

  if (p->config != NULL)
    {
      if (p->decoder != NULL)
        {
          if (decode_from_stream (p->stream, p->decoder) < 0)
            {
              g_warning ("failed decoding WebP image file");
              cleanup (operation);
              return FALSE;
            }

          g_input_stream_close (G_INPUT_STREAM (p->stream), NULL, NULL);
          g_clear_object (&p->stream);

          WebPIDelete (p->decoder);
          p->decoder = NULL;
        }

      gegl_buffer_set (output, result, 0, p->format,
                       p->config->output.u.RGBA.rgba,
                       p->config->output.u.RGBA.stride);
    }

  return FALSE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
finalize(GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data != NULL)
    {
      cleanup (GEGL_OPERATION (object));
      if (o->user_data != NULL)
        g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS(klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:webp-load",
    "title",        _("WebP File Loader"),
    "categories"  , "hidden",
    "description" , _("WebP image loader."),
    NULL);

  gegl_operation_handlers_register_loader (
    "image/webp", "gegl:webp-load");
  gegl_operation_handlers_register_loader (
    ".webp", "gegl:webp-load");
}

#endif
