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
 * Copyright 2011 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Target path and filename, use '-' for stdout"))

property_int (quality, _("Quality"), 90)
  description (_("JPEG compression quality (between 1 and 100)"))
  value_range (1, 100)

property_int (smoothing, _("Smoothing"), 0)
  description(_("Smoothing factor from 1 to 100; 0 disables smoothing"))
  value_range (0, 100)

property_boolean (optimize, _("Optimize"), TRUE)
  description (_("Use optimized huffman tables"))
property_boolean (progressive, _("Progressive"), TRUE)
  description (_("Create progressive JPEG images"))
property_boolean (grayscale, _("Grayscale"), FALSE)
  description (_("Create a grayscale (monochrome) image"))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME jpg_save
#define GEGL_OP_C_SOURCE jpg-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <stdio.h> /* jpeglib.h needs FILE... */
#include <jpeglib.h>

static const gsize buffer_size = 4096;

static void
init_buffer (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  guchar *buffer;

  buffer = g_try_new (guchar, buffer_size);

  g_assert (buffer != NULL);

  dest->next_output_byte = buffer;
  dest->free_in_buffer = buffer_size;
}

static boolean
write_to_stream (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  GOutputStream *stream = (GOutputStream *) cinfo->client_data;
  GError *error = NULL;
  guchar *buffer;
  gsize size;
  gboolean success;
  gsize written;

  g_assert (stream);

  size = buffer_size - dest->free_in_buffer;
  buffer = (guchar *) dest->next_output_byte - size;

  success = g_output_stream_write_all (G_OUTPUT_STREAM (stream),
                                       (const void *) buffer, buffer_size, &written,
                                       NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return FALSE;
    }

  dest->next_output_byte = buffer;
  dest->free_in_buffer = buffer_size;

  return TRUE;
}

static void
close_stream (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  GOutputStream *stream = (GOutputStream *) cinfo->client_data;
  GError *error = NULL;
  guchar *buffer;
  gsize size;
  gboolean success;
  gsize written;
  gboolean closed;

  g_assert (stream);

  size = buffer_size - dest->free_in_buffer;
  buffer = (guchar *) dest->next_output_byte - size;

  success = g_output_stream_write_all (G_OUTPUT_STREAM (stream),
                                       (const void *) buffer, size, &written,
                                       NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  closed = g_output_stream_close (G_OUTPUT_STREAM (stream),
                                  NULL, &error);
  if (!closed)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_free (buffer);

  dest->next_output_byte = NULL;
  dest->free_in_buffer = 0;
}

static gint
export_jpg (GeglOperation               *operation,
            GeglBuffer                  *input,
            const GeglRectangle         *result,
            struct jpeg_compress_struct  cinfo,
            gint                         quality,
            gint                         smoothing,
            gboolean                     optimize,
            gboolean                     progressive,
            gboolean                     grayscale)
{
  gint     src_x, src_y;
  gint     width, height;
  JSAMPROW row_pointer[1];
  const Babl *format;

  src_x = result->x;
  src_y = result->y;
  width = result->width - result->x;
  height = result->height - result->y;

  cinfo.image_width = width;
  cinfo.image_height = height;

  if (!grayscale)
    {
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
    }
  else
    {
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    }

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
  cinfo.smoothing_factor = smoothing;
  cinfo.optimize_coding = optimize;
  if (progressive)
    jpeg_simple_progression (&cinfo);

  /* Use 1x1,1x1,1x1 MCUs and no subsampling */
  cinfo.comp_info[0].h_samp_factor = 1;
  cinfo.comp_info[0].v_samp_factor = 1;

  if (!grayscale)
    {
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
    }

  /* No restart markers */
  cinfo.restart_interval = 0;
  cinfo.restart_in_rows = 0;

  jpeg_start_compress (&cinfo, TRUE);

  if (!grayscale)
    {
      format = babl_format ("R'G'B' u8");
      row_pointer[0] = g_malloc (width * 3);
    }
  else
    {
      format = babl_format ("Y' u8");
      row_pointer[0] = g_malloc (width);
    }

  while (cinfo.next_scanline < cinfo.image_height) {
    GeglRectangle rect;

    rect.x = src_x;
    rect.y = src_y + cinfo.next_scanline;
    rect.width = width;
    rect.height = 1;

    gegl_buffer_get (input, &rect, 1.0, format,
                     row_pointer[0], GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);

    jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);

  g_free (row_pointer[0]);

  return 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         int                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_destination_mgr dest;
  GOutputStream *stream;
  GFile *file = NULL;
  gboolean status = TRUE;
  GError *error = NULL;

  cinfo.err = jpeg_std_error (&jerr);

  jpeg_create_compress (&cinfo);

  stream = gegl_gio_open_output_stream (NULL, o->path, &file, &error);
  if (stream == NULL)
    {
      status = FALSE;
      g_warning ("%s", error->message);
      goto cleanup;
    }

  dest.init_destination = init_buffer;
  dest.empty_output_buffer = write_to_stream;
  dest.term_destination = close_stream;

  cinfo.client_data = stream;
  cinfo.dest = &dest;

  if (export_jpg (operation, input, result, cinfo,
                  o->quality, o->smoothing, o->optimize, o->progressive, o->grayscale))
    {
      status = FALSE;
      g_warning("could not export JPEG file");
      goto cleanup;
    }

cleanup:
  jpeg_destroy_compress (&cinfo);

  if (stream != NULL)
    g_clear_object (&stream);

  if (file != NULL)
    g_clear_object (&file);

  return  status;
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
    "name",          "gegl:jpg-save",
    "title",       _("JPEG File Saver"),
    "categories",    "output",
    "description", _("JPEG image saver, using libjpeg"),
    NULL);

  gegl_operation_handlers_register_saver (
    ".jpeg", "gegl:jpg-save");
  gegl_operation_handlers_register_saver (
    ".jpg", "gegl:jpg-save");
}

#endif
