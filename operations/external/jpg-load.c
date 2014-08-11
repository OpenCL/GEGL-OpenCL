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

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_FILE       "jpg-load.c"

#include "gegl-op.h"
#include <stdio.h>
#include <jpeglib.h>

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


static gint
gegl_jpg_load_query_jpg (const gchar *path,
                         gint        *width,
                         gint        *height,
                         const Babl  **out_format)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *infile;
  gint                           status = 0;
  const Babl *format = NULL;

  if ((infile = fopen (path, "rb")) == NULL)
    {
      /*g_warning ("unable to open %s for jpeg import", path);*/
      return -1;
    }

  jpeg_create_decompress (&cinfo);
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_stdio_src (&cinfo, infile);

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

  fclose (infile);
  return status;
}

static gint
gegl_jpg_load_buffer_import_jpg (GeglBuffer  *gegl_buffer,
                                 const gchar *path,
                                 gint         dest_x,
                                 gint         dest_y)
{
  gint row_stride;
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *infile;
  JSAMPARRAY                     buffer;
  const Babl                    *format;
  GeglRectangle                  write_rect;

  if ((infile = fopen (path, "rb")) == NULL)
    {
      g_warning ("unable to open %s for jpeg import", path);
      return -1;
    }

  jpeg_create_decompress (&cinfo);
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_stdio_src (&cinfo, infile);

  (void) jpeg_read_header (&cinfo, TRUE);
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
  const gboolean is_inverted_cmyk = (format == babl_format("CMYK u8"));

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
  fclose (infile);

  return 0;
}

static GeglRectangle
gegl_jpg_load_get_bounding_box (GeglOperation *operation)
{
  gint width, height;
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  const Babl *format = NULL;
  const gint status = gegl_jpg_load_query_jpg (o->path, &width, &height, &format);

  if (format)
    gegl_operation_set_format (operation, "output", format);

  if (status)
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

  if (gegl_jpg_load_buffer_import_jpg (output, o->path, 0, 0))
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);

      return FALSE;
    }

  return  TRUE;
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
