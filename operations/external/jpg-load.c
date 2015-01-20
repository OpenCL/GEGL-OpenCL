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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI of file to load"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE jpg-load.c

#include "gegl-op.h"
#include <stdio.h>
#include <jpeglib.h>
#include <gegl-gio-private.h>

static const gchar *
jpeg_colorspace_name(J_COLOR_SPACE space)
{
    static const gchar * const names[] = {
        "Unknown",
        "Grayscale",
        "RGB",
        "YCbCr",
        "CMYK",
        "YCCK"
    };
    const gint n_valid_names = G_N_ELEMENTS(names);
    const gint idx = (space > 0 && space < n_valid_names) ? (gint)space : 0;
    return names[idx];
}

static const Babl *
babl_from_jpeg_colorspace(J_COLOR_SPACE space)
{
    // XXX: assumes bitdepth == 8
  const Babl *format = NULL;
  if (space == JCS_GRAYSCALE)
    format = babl_format ("Y' u8");
  else if (space == JCS_RGB)
    format = babl_format ("R'G'B' u8");
  else if (space == JCS_CMYK) {
    format = babl_format("CMYK u8");
  }

  return format;
}

typedef struct {
    GInputStream *stream;
    gchar *buffer;
    gsize buffer_size;
} GioSource;

static boolean
gio_source_fill(j_decompress_ptr cinfo)
{
    struct jpeg_source_mgr* src = cinfo->src;
    GioSource *self = (GioSource *)cinfo->client_data;
    GError *err = NULL;

    const gssize bytes_read = g_input_stream_read(self->stream, self->buffer,
                                                  self->buffer_size, NULL, &err);
    if (!err)
      {
        src->next_input_byte = (JOCTET*)self->buffer;
        src->bytes_in_buffer = bytes_read;
      }
    else
      {
        g_print("%s: %s\n", __PRETTY_FUNCTION__, err->message);
      }
    

    /* FIXME: needed for EOF?
    static const JOCTET EOI_BUFFER[ 2 ] = { (JOCTET)0xFF, (JOCTET)JPEG_EOI };
    src->next_input_byte = EOI_BUFFER;
    src->bytes_in_buffer = sizeof( EOI_BUFFER );
    */

    return TRUE;
}

static void
gio_source_skip(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*)cinfo->src;
    GioSource *self = (GioSource *)cinfo->client_data;

    if (num_bytes < src->bytes_in_buffer)
      {
        // just skip inside buffer
        src->next_input_byte += (size_t)num_bytes;
        src->bytes_in_buffer -= (size_t)num_bytes;
      }
    else
      {
        // skip in stream, discard whole buffer
        GError *err = NULL;
        const gssize bytes_to_skip = num_bytes-src->bytes_in_buffer;
        const gssize skipped = g_input_stream_skip(self->stream, bytes_to_skip, NULL, &err);
        if (err)
          {
            g_printerr("%s: skipped=%ld, err=%s\n", __PRETTY_FUNCTION__, skipped, err->message);
            g_error_free(err);
          }
        src->bytes_in_buffer = 0;
        src->next_input_byte = NULL;
      }

}

static void
gio_source_init(j_decompress_ptr cinfo)
{
    GioSource *self = (GioSource *)cinfo->client_data;
    self->buffer = g_new(gchar, self->buffer_size);
}
 
static void
gio_source_destroy(j_decompress_ptr cinfo)
{
    GioSource *self = (GioSource *)cinfo->client_data;
    g_free(self->buffer);
}

static void
gio_source_enable(j_decompress_ptr cinfo, struct jpeg_source_mgr* src, GioSource *data)
{
    src->resync_to_restart = jpeg_resync_to_restart;

    src->init_source = gio_source_init;
    src->fill_input_buffer = gio_source_fill;
    src->skip_input_data = gio_source_skip;
    src->term_source = gio_source_destroy;

    // force fill at once
    src->bytes_in_buffer = 0;
    src->next_input_byte = (JOCTET*)NULL;

    //g_assert(!cinfo->client_data);
    cinfo->client_data = data;
    cinfo->src = src;
}


static gint
gegl_jpg_load_query_jpg (GInputStream *stream,
                         gint        *width,
                         gint        *height,
                         const Babl  **out_format)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  struct jpeg_source_mgr         src;
  gint                           status = 0;
  const Babl *format = NULL;
  GioSource gio_source = { stream, NULL, 1024 };

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);

  gio_source_enable(&cinfo, &src, &gio_source);

  (void) jpeg_read_header (&cinfo, TRUE);

  format = babl_from_jpeg_colorspace(cinfo.out_color_space);
  if (width)
    *width = cinfo.image_width;
  if (height)
    *height = cinfo.image_height;
  if (out_format)
    *out_format = format;
  if (!format)
    {
      g_warning ("attempted to load JPEG with unsupported color space: '%s'",
                 jpeg_colorspace_name(cinfo.out_color_space));
      status = -1;
    }

  jpeg_destroy_decompress (&cinfo);

  return status;
}

static gint
gegl_jpg_load_buffer_import_jpg (GeglBuffer  *gegl_buffer,
                                 GInputStream *stream,
                                 gint         dest_x,
                                 gint         dest_y)
{
  gint row_stride;
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  struct jpeg_source_mgr         src;
  JSAMPARRAY                     buffer;
  const Babl                    *format;
  GeglRectangle                  write_rect;
  gboolean                       is_inverted_cmyk = FALSE;
  GioSource gio_source = { stream, NULL, 1024 };

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);

  gio_source_enable(&cinfo, &src, &gio_source);

  (void) jpeg_read_header (&cinfo, TRUE);

  /* This is the most accurate method and could be the fastest too. But
   * the results may vary on different platforms due to different
   * rounding behavior and precision.
   */
  cinfo.dct_method = JDCT_FLOAT;

  (void) jpeg_start_decompress (&cinfo);

  format = babl_from_jpeg_colorspace(cinfo.out_color_space);
  if (!format)
    {
      g_warning ("attempted to load JPEG with unsupported color space: '%s'",
                 jpeg_colorspace_name(cinfo.out_color_space));
      jpeg_destroy_decompress (&cinfo);
      return -1;
    }

  row_stride = cinfo.output_width * cinfo.output_components;

  if ((row_stride) % 2)
    (row_stride)++;

  /* allocated with the jpeg library, and freed with the decompress context */
  buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  write_rect.x = dest_x;
  write_rect.y = dest_y;
  write_rect.width  = cinfo.output_width;
  write_rect.height = 1;

  // Most CMYK JPEG files are produced by Adobe Photoshop. Each component is stored where 0 means 100% ink
  // However this might not be case for all. Gory details: https://bugzilla.mozilla.org/show_bug.cgi?id=674619
  is_inverted_cmyk = (format == babl_format("CMYK u8"));

  while (cinfo.output_scanline < cinfo.output_height)
    {
      jpeg_read_scanlines (&cinfo, buffer, 1);

      if (is_inverted_cmyk) {
        for (int i=0; i<row_stride; i++) {
            buffer[0][i] = 255-buffer[0][i];
        }
      }

      gegl_buffer_set (gegl_buffer, &write_rect, 0,
                       format, buffer[0],
                       GEGL_AUTO_ROWSTRIDE);

      write_rect.y += 1;
    }

  jpeg_destroy_decompress (&cinfo);

  return 0;
}

static GeglRectangle
gegl_jpg_load_get_bounding_box (GeglOperation *operation)
{
  gint width, height;
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  const Babl *format = NULL;
  GFile *file = NULL;
  GError *err = NULL;
  gint status = -1;

  GInputStream *stream = gegl_gio_open_input_stream(o->uri, o->path, &file, &err);
  if (!stream)
    return (GeglRectangle) {0, 0, 0, 0};
  status = gegl_jpg_load_query_jpg (stream, &width, &height, &format);
  g_input_stream_close(stream, NULL, NULL);

  if (format)
    gegl_operation_set_format (operation, "output", format);

  g_object_unref(stream);
  if (file) g_object_unref(file);
  if (err || status)
    return (GeglRectangle) {0, 0, 0, 0};
  else
    return (GeglRectangle) {0, 0, width, height};
}

static gboolean
gegl_jpg_load_process (GeglOperation       *operation,
                       GeglBuffer          *output,
                       const GeglRectangle *result,
                       gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GFile *file = NULL;
  GError *err = NULL;
  gint status = -1;
  GInputStream *stream = gegl_gio_open_input_stream(o->uri, o->path, &file, &err);
  if (!stream)
    return FALSE;
  status = gegl_jpg_load_buffer_import_jpg(output, stream, 0, 0);
  g_input_stream_close(stream, NULL, NULL);

  if (err)
    {
      g_warning ("%s failed to open file %s for reading: %s",
        G_OBJECT_TYPE_NAME (operation), o->path, err->message);
      g_object_unref(stream);
      return FALSE;
    }

  g_object_unref(stream);
  return status != 1;
}

static GeglRectangle
gegl_jpg_load_get_cached_region (GeglOperation       *operation,
                                 const GeglRectangle *roi)
{
  return gegl_jpg_load_get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = gegl_jpg_load_process;
  operation_class->get_bounding_box = gegl_jpg_load_get_bounding_box;
  operation_class->get_cached_region = gegl_jpg_load_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:jpg-load",
    "title",       _("JPEG File Loader"),
    "categories",  "hidden",
    "description", _("JPEG image loader using libjpeg"),
    NULL);

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_extension_handler_register (".jpg", "gegl:jpg-load");
  gegl_extension_handler_register (".jpeg", "gegl:jpg-load");
/*  done = TRUE; */
}
#endif
