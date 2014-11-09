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
 *           2006 Dominik Ernst <dernst@gmx.de>
 *           2006 Kevin Cozens <kcozens@cvs.gnome.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load."))
// TODO: use dedicated property spec for URI
property_string (uri, _("URI"), "")
  description (_("URI for file to load."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_FILE       "png-load.c"

#include "gegl-op.h"
#include <png.h>

static void
read_fn(png_structp png_ptr, png_bytep buffer, png_size_t length)
{
  GError *err = NULL;
  GInputStream *stream = G_INPUT_STREAM(png_get_io_ptr(png_ptr));
  gboolean success = FALSE;
  gsize bytes_read = 0;
  g_assert(stream);

  success = g_input_stream_read_all(stream, buffer, length, &bytes_read, NULL, &err);
  if (!success) {
    g_printerr("gegl:load-png %s: %s\n", __PRETTY_FUNCTION__, err->message);
  }
}

static void
error_fn(png_structp png_ptr, png_const_charp msg)
{
  g_printerr("LIBPNG ERROR: %s", msg);
}

static GFile * open_png(const gchar *uri, const gchar *path)
{
  GError *err = NULL;
  GFile *infile = NULL;
  GInputStream *fis = NULL;
  const size_t hdr_size=8;
  gssize hdr_read_size;
  unsigned char header[hdr_size];

  g_return_val_if_fail(uri || path, NULL);

  if (strlen(uri) > 0)
    {
      infile = g_file_new_for_uri(uri);
    }
  else if (g_strcmp0 (path, "-") == 0)
    {
      //infile = stdin; // FIXME: implement
      g_assert(FALSE);
    }
  else
    {
      infile = g_file_new_for_path(path);
    }
  g_return_val_if_fail(infile, NULL);

  fis = G_INPUT_STREAM(g_file_read(infile, NULL, &err));
  hdr_read_size = g_input_stream_read(G_INPUT_STREAM(fis), header, hdr_size, NULL, &err);
  if (hdr_read_size != hdr_size)
    {
      g_input_stream_close(fis, NULL, NULL);
      g_object_unref(fis);

      if (err) {
        g_printerr("%s", err->message);
      } else {
        g_warning ("%s is too short for a png file, only %lu bytes.",
                   path, (unsigned long)hdr_read_size);
      }

      return NULL;
    }

  if (png_sig_cmp (header, 0, hdr_size))
    {
      g_input_stream_close(fis, NULL, NULL);
      g_object_unref(fis);
      g_warning ("%s is not a png file", path);
      return NULL;
    }

  g_input_stream_close(fis, NULL, NULL); // consumer should open new
  g_object_unref(fis);
  return infile;
}

static gint
gegl_buffer_import_png (GeglBuffer  *gegl_buffer,
                        GInputStream *stream,
                        gint         dest_x,
                        gint         dest_y,
                        gint        *ret_width,
                        gint        *ret_height,
                        gpointer     format)
{
  gint           width;
  gint           bit_depth;
  gint           bpp;
  gint           number_of_passes=1;
  png_uint_32    w;
  png_uint_32    h;
  png_structp    load_png_ptr;
  png_infop      load_info_ptr;
  guchar        *pixels;
  /*png_bytep     *rows;*/


  unsigned   int i;
  png_bytep  *row_p = NULL;

  g_return_val_if_fail(stream, -1);

  load_png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, error_fn, NULL);

  if (!load_png_ptr)
    {
      return -1;
    }

  load_info_ptr = png_create_info_struct (load_png_ptr);
  if (!load_info_ptr)
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
      return -1;
    }

  if (setjmp (png_jmpbuf (load_png_ptr)))
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
     if (row_p)
        g_free (row_p);
      return -1;
    }

  png_set_read_fn(load_png_ptr, stream, read_fn);

//  png_set_sig_bytes (load_png_ptr, 8);
  png_read_info (load_png_ptr, load_info_ptr);
  {
    int color_type;
    int interlace_type;

    png_get_IHDR (load_png_ptr,
                  load_info_ptr,
                  &w, &h,
                  &bit_depth,
                  &color_type,
                  &interlace_type,
                  NULL, NULL);
    width = w;
    if (ret_width)
      *ret_width = w;
    if (ret_height)
      *ret_height = h;

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      {
        png_set_expand (load_png_ptr);
        bit_depth = 8;
      }

    if (png_get_valid (load_png_ptr, load_info_ptr, PNG_INFO_tRNS))
      {
        png_set_tRNS_to_alpha (load_png_ptr);
        color_type |= PNG_COLOR_MASK_ALPHA;
      }

    switch (color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          bpp = 1;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          bpp = 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          bpp = 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          bpp = 4;
          break;
        case (PNG_COLOR_TYPE_PALETTE | PNG_COLOR_MASK_ALPHA):
          bpp = 4;
          break;
        case PNG_COLOR_TYPE_PALETTE:
          bpp = 3;
          break;
        default:
          g_warning ("color type mismatch");
          png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
          return -1;
      }

    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb (load_png_ptr);

    if (bit_depth == 16)
      bpp = bpp << 1;

#if BYTE_ORDER == LITTLE_ENDIAN
    if (bit_depth == 16)
      png_set_swap (load_png_ptr);
#endif

    if (interlace_type == PNG_INTERLACE_ADAM7)
      number_of_passes = png_set_interlace_handling (load_png_ptr);

    if (png_get_valid (load_png_ptr, load_info_ptr, PNG_INFO_gAMA))
      {
        gdouble gamma;
        png_get_gAMA (load_png_ptr, load_info_ptr, &gamma);
        png_set_gamma (load_png_ptr, 2.2, gamma);
      }
    else
      {
        png_set_gamma (load_png_ptr, 2.2, 0.45455);
      }

    png_read_update_info (load_png_ptr, load_info_ptr);
  }

  pixels = g_malloc0 (width*bpp);

  {
    gint           pass;
    GeglRectangle  rect;

    for (pass=0; pass<number_of_passes; pass++)
      {
        for(i=0; i<h; i++)
          {
            gegl_rectangle_set (&rect, 0, i, width, 1);

            if (pass != 0)
              gegl_buffer_get (gegl_buffer, &rect, 1.0, format, pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

            png_read_rows (load_png_ptr, &pixels, NULL, 1);
            gegl_buffer_set (gegl_buffer, &rect, 0, format, pixels,
                             GEGL_AUTO_ROWSTRIDE);
          }
      }
  }


  png_read_end (load_png_ptr, NULL);
  png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);

  g_free (pixels);

  return 0;
}

static gint query_png (GInputStream *stream,
                       gint        *width,
                       gint        *height,
                       gpointer    *format)
{
  png_uint_32   w;
  png_uint_32   h;
  png_structp   load_png_ptr;
  png_infop     load_info_ptr;

  png_bytep  *row_p = NULL;

  g_return_val_if_fail(stream, -1);

  load_png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, error_fn, NULL);
  if (!load_png_ptr)
    {
      return -1;
    }

  load_info_ptr = png_create_info_struct (load_png_ptr);
  if (!load_info_ptr)
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
      return -1;
    }

  if (setjmp (png_jmpbuf (load_png_ptr)))
    {
     png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
     if (row_p)
        g_free (row_p);
      return -1;
    }

  png_set_read_fn(load_png_ptr, stream, read_fn);
//  png_set_sig_bytes (load_png_ptr, 8);
  png_read_info (load_png_ptr, load_info_ptr);
  {
    int bit_depth;
    int color_type;
    gchar format_string[32];

    png_get_IHDR (load_png_ptr,
                  load_info_ptr,
                  &w, &h,
                  &bit_depth,
                  &color_type,
                  NULL, NULL, NULL);
    *width = w;
    *height = h;

    if (png_get_valid (load_png_ptr, load_info_ptr, PNG_INFO_tRNS))
      color_type |= PNG_COLOR_MASK_ALPHA;

    if (color_type & PNG_COLOR_TYPE_RGB)
      {
        if (color_type & PNG_COLOR_MASK_ALPHA)
          strcpy (format_string, "R'G'B'A ");
        else
          strcpy (format_string, "R'G'B' ");
      }
    else if ((color_type & PNG_COLOR_TYPE_GRAY) == PNG_COLOR_TYPE_GRAY)
      {
        if (color_type & PNG_COLOR_MASK_ALPHA)
          strcpy (format_string, "Y'A ");
        else
          strcpy (format_string, "Y' ");
      }
    else if (color_type & PNG_COLOR_TYPE_PALETTE)
      {
        if (color_type & PNG_COLOR_MASK_ALPHA)
          strcpy (format_string, "R'G'B'A ");
        else
          strcpy (format_string, "R'G'B' ");
      }
    else
      {
        g_warning ("color type mismatch");
        png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
        return -1;
      }

    if (bit_depth <= 8)
      {
        strcat (format_string, "u8");
      }
    else if(bit_depth == 16)
      {
        strcat (format_string, "u16");
      }
    else
      {
        g_warning ("bit depth mismatch");
        png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
        return -1;
      }

    *format = (void*)babl_format (format_string);
  }
  png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
  return 0;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  gint          width, height;
  gint          status;
  gpointer      format;
  GError *err = NULL;

  GFile *infile = open_png(o->uri, o->path);
  GInputStream *stream = G_INPUT_STREAM(g_file_read(infile, NULL, &err));
  status = query_png (stream, &width, &height, &format);
  g_input_stream_close(stream, NULL, NULL);

  if (status)
    {
      width = 0;
      height = 0;
    }

  gegl_operation_set_format (operation, "output", format);
  result.width  = width;
  result.height  = height;

  g_object_unref(infile);
  g_object_unref(stream);
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gint        problem;
  gpointer    format;
  gint        width, height;
  GFile *infile = open_png(o->uri, o->path);
  GError *err = NULL;
  
  GInputStream *stream = G_INPUT_STREAM(g_file_read(infile, NULL, &err));

  problem = query_png (stream, &width, &height, &format);
  if (problem)
    {
      g_object_unref(infile);
      g_object_unref(stream);
      g_warning ("%s is %s really a PNG file?",
      G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }

  // TEMP: avoding having to recreate stream. import_png should do query??
  g_input_stream_close(stream, NULL, NULL);
  g_object_unref(stream);
  stream = G_INPUT_STREAM(g_file_read(infile, NULL, &err));

  problem = gegl_buffer_import_png (output, stream, 0, 0,
                                    &width, &height, format);

  if (problem)
    {
      g_object_unref(infile);
      g_object_unref(stream);
      g_warning ("%s failed to open file %s for reading.",
                 G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }
  g_object_unref(infile);
  g_object_unref(stream);
  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:png-load",
    "title",        _("PNG File Loader"),
    "categories",   "hidden",
    "description",  _("PNG image loader."),
    NULL);

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_extension_handler_register (".png", "gegl:png-load");
/*  done = TRUE; */
}

#endif
