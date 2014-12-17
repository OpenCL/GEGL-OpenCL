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
#else
#include <gio/gunixinputstream.h>
#endif

static const gint data_prefix_len = strlen("data:");

// Supports data embedded in the URI itself
// http://en.wikipedia.org/wiki/Data_URI_scheme
// https://developer.mozilla.org/en-US/docs/Web/HTTP/data_URIs
static GInputStream *
input_stream_datauri(const gchar *uri)
{
  GInputStream *stream = NULL;

  // Determine data format
  const gchar * header_end = g_strstr_len (uri, -1, ",");
  g_return_val_if_fail(header_end, NULL);

  const gint header_len = header_end - uri - data_prefix_len;
  gchar *header = g_strndup(uri+data_prefix_len, header_len);  

  gchar **header_items = g_strsplit(header, ";", 3);
  gint header_items_no = -1;
  while (header_items[++header_items_no]) {
  }

  // Note: header_item[0] is content-type
  const gboolean is_base64 = header_items_no > 1 && g_strcmp0(header_items[1], "base64") == 0;
  g_free(header);
  g_strfreev(header_items);

  const gchar *raw_data = uri+data_prefix_len+header_len;

  if (is_base64) {
    gsize len = 0;
    gchar * data = g_base64_decode (raw_data, &len);
    stream = g_memory_input_stream_new_from_data (data, len, g_free);
  } else {
    gchar *data = g_strdup(raw_data);
    const gint len = strlen(data);
    stream = g_memory_input_stream_new_from_data (data, len, g_free);
  }

  return stream;
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
      if (g_str_has_prefix(uri, "data:"))
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
