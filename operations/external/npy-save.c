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
 * Copyright Dov Grobgeld 2013 <dov.grobgeld (a) gmail.com>
 *
 * This operation saves a buffer in the npy file format. It may be
 * read into python as follows:
 *
 *   import numpy
 *   img = numpy.load('image.npy')
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Target path and filename, use '-' for stdout"))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME npy_save
#define GEGL_OP_C_SOURCE npy-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <string.h>

static gsize
write_to_stream (GOutputStream     *stream,
                 const gchar*       data,
                 gsize              size)
{
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

  return written;
}

static gint
write_header (GOutputStream *stream,
              gint           width,
              gint           height,
              gint           nb_components,
              gint           bytes_per_pixel)
{
  gchar *header;
  gsize length;

  // Write header and version number (1.0)
  write_to_stream (stream, "\x93NUMPY\x01\x00", 8);

  if (nb_components == 3)
    {
      header = g_strdup_printf ("{'descr': '<f4', 'fortran_order': False,"
                                " 'shape': (%d, %d, 3), } \n", height, width);
    }
  else
    {
      header = g_strdup_printf ("{'descr': '<f4', 'fortran_order': False,"
                                " 'shape': (%d, %d), } \n", height, width);
    }

  length = strlen (header);

  write_to_stream (stream, (const char *) &length, 2);
  write_to_stream (stream, header, length);

  g_free (header);
  return 0;
}

static gint
save_array (GOutputStream       *stream,
            GeglBuffer          *input,
            const GeglRectangle *result,
            const Babl          *format)
{
  gint bytes_per_pixel, bytes_per_row;
  gint x = result->x, y = result->y;
  gint width = result->width - result->x;
  gint height = result->height - result->y;
  gint column_stride = 32;
  gchar *buffer;
  gint nb_components ,row;

  nb_components = babl_format_get_n_components (format);
  bytes_per_pixel = babl_format_get_bytes_per_pixel (format);

  write_header (stream, width, height, nb_components, bytes_per_pixel);

  bytes_per_row = bytes_per_pixel * width;

  buffer = g_try_new (gchar, bytes_per_row * column_stride);

  g_assert (buffer != NULL);

  for (row = 0; row < height; row += column_stride)
    {
      GeglRectangle tile = { x, 0, width, 0 };

      tile.y = y + row;
      tile.height = MIN (column_stride, height - row);

      gegl_buffer_get (input, &tile, 1.0, format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      write_to_stream (stream, buffer, bytes_per_row * tile.height);
    }

  g_free (buffer);
  return 0;
}

static gboolean
export_numpy (GeglOperation       *operation,
              GeglBuffer          *input,
              const GeglRectangle *result,
              GOutputStream       *stream)
{
  const Babl *input_format, *output_format;
  gint nb_components;

  input_format = gegl_buffer_get_format (input);
  nb_components = babl_format_get_n_components (input_format);
  if (nb_components >= 3)
    {
      output_format = babl_format ("RGB float");
      nb_components = 3;
    }
  else
    {
      output_format = babl_format ("Y float");
      nb_components = 1;
    }

  return !save_array (stream, input, result, output_format);
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

  if (!export_numpy (operation, input, result, stream))
    {
      status = FALSE;
      g_warning ("could not export NumPy file");
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
    "name",          "gegl:npy-save",
    "title",       _("NumPy File Saver"),
    "categories",    "output",
    "description", _("NumPy (Numerical Python) image saver"),
    NULL);

  gegl_operation_handlers_register_saver (
    ".npy", "gegl:npy-save");
}

#endif
