/* This file is part of GEGL
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
 * Copyright 2014 Jon Nordby, The Grid <jononor@gmail.com>
 */

#include <gegl-gio-private.h>
#include <string.h>
#include <stdio.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <gio/gwin32inputstream.h>
#include <gio/gwin32outputstream.h>
#else
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#endif

/**
 * datauri_parse_header:
 * @uri: URI to parse
 * @raw_data_out: (out) (transfer full): return location for pointer to raw data, following header
 * @header_items_no_out: (out) (transfer none): return location for number of items parsed from header
 *
 * Return value: (transfer full): header_item[0] is content-type, header_items[1] is encoding, free with g_strfreev()
 *
 * Note: currently private API
 */
static gchar **
datauri_parse_header(const gchar *uri, gchar **raw_data_out, gint *header_items_no_out)
{
  // Determine data format
  const gchar * header_end = g_strstr_len (uri, -1, ",");
  const gint data_prefix_len = strlen("data:");

  const gint header_len = header_end - uri - data_prefix_len;
  gchar *header = g_strndup(uri+data_prefix_len, header_len);  

  gchar **header_items = g_strsplit(header, ";", 3);
  gint header_items_no = -1;
  while (header_items[++header_items_no]) {
  }
  g_free(header);

  if (header_items_no_out)
    {
      *header_items_no_out = header_items_no;
    }

  if (raw_data_out)
    {
      *raw_data_out = (gchar *)uri+data_prefix_len+header_len;
    }
  return header_items;
}

// Supports data embedded in the URI itself
// http://en.wikipedia.org/wiki/Data_URI_scheme
// https://developer.mozilla.org/en-US/docs/Web/HTTP/data_URIs
static GInputStream *
input_stream_datauri(const gchar *uri)
{
  GInputStream *stream = NULL;
  gchar * raw_data = NULL;
  gint header_items_no = 0;
  gchar **header_items = datauri_parse_header(uri, &raw_data, &header_items_no);
  const gboolean is_base64 = header_items_no > 1 && g_strcmp0(header_items[1], "base64") == 0;

  if (is_base64) {
    gsize len = 0;
    guchar * data = g_base64_decode (raw_data, &len);
    stream = g_memory_input_stream_new_from_data (data, len, g_free);
  } else {
    gchar *data = g_strdup(raw_data);
    const gint len = strlen(data);
    stream = g_memory_input_stream_new_from_data (data, len, g_free);
  }

  g_strfreev(header_items);
  return stream;
}

gchar *
gegl_gio_datauri_get_content_type(const gchar *uri)
{
  gchar *content_type = NULL;
  gint header_items_no = 0;
  gchar **header_items = datauri_parse_header(uri, NULL, &header_items_no);
  if (header_items_no)
    content_type = g_strdup(header_items[0]);
  g_strfreev(header_items);
  return content_type;
}

gboolean
gegl_gio_uri_is_datauri(const gchar *uri)
{
    return g_str_has_prefix(uri, "data:");
}

/**
 * gegl_gio_open_input_stream:
 * @uri: (allow none) URI to open. @uri is preferred over @path if both are set
 * @path: (allow none) path to open. 
 * @out_file: (out) (transfer full): return location for GFile, if stream is for a file
 *
 * Return value: (transfer full): A new #GInputStream, free with g_object_unref()
 *
 * Note: currently private API
 */
GInputStream *
gegl_gio_open_input_stream(const gchar *uri, const gchar *path, GFile **out_file, GError **err)
{
  GFile *infile = NULL;
  GInputStream *fis = NULL;
  g_return_val_if_fail(uri || path, NULL);
  g_return_val_if_fail(out_file, NULL);

  if (path && g_strcmp0(path, "-") == 0)
    {
      const gboolean close_fd = FALSE;
      infile = NULL;
#ifdef G_OS_WIN32
      fis = g_win32_input_stream_new (GetStdHandle (STD_INPUT_HANDLE), close_fd);
#else
      fis = g_unix_input_stream_new(STDIN_FILENO, close_fd);
#endif
    }
  else if (uri && strlen(uri) > 0)
    {
      if (gegl_gio_uri_is_datauri(uri))
        {
          fis = input_stream_datauri(uri);
        }
      else
        {
          infile = g_file_new_for_uri(uri);
        }
    }
  else if (path && strlen(path) > 0)
    {
      infile = g_file_new_for_path(path);
    }
  else {
    return NULL;
  }

  if (infile)
    {
      g_assert(!fis);
      fis = G_INPUT_STREAM(g_file_read(infile, NULL, err));
      *out_file = infile;
    }

  return fis;
}

/**
 * gegl_gio_open_output_stream:
 * @uri: (allow none) URI to open. @uri is preferred over @path if both are set.
 * @path: (allow none) path to open.
 * @out_file: (out) (transfer full): return location for GFile, if stream is for a file.
 *
 * Return value: (transfer full): A new #GOutputStream, free with g_object_unref().
 *
 * Note: currently private API.
 */
GOutputStream *
gegl_gio_open_output_stream(const gchar *uri, const gchar *path, GFile **out_file, GError **err)
{
  GOutputStream *stream = NULL;
  const gboolean close = FALSE;
  GFile *file = NULL;

  g_return_val_if_fail(uri || path, NULL);
  g_return_val_if_fail(out_file, NULL);

  if (path && g_strcmp0(path, "-") == 0)
    {
#ifdef G_OS_WIN32
      stream = g_win32_output_stream_new(GetStdHandle(STD_OUTPUT_HANDLE), close);
#else
      stream = g_unix_output_stream_new(STDOUT_FILENO, close);
#endif
    }
  else if (uri && strlen(uri) > 0)
    file = g_file_new_for_uri(uri);
  else if (path && strlen(path) > 0)
    file = g_file_new_for_path(path);
  else
    return NULL;

  if (file != NULL)
    {
      g_assert(stream == NULL);

      stream = G_OUTPUT_STREAM(g_file_replace(file, NULL,
                                              FALSE, G_FILE_CREATE_NONE,
                                              NULL, err));

      *out_file = file;
    }

  return stream;
}
