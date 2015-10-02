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
 * Copyright 2015 Martin Blanchard <tchaik@gmx.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI for file to load"))

property_int(directory, _("Directory"), 1)
  description (_("Image file directory (subfile)"))
  value_range (1, G_MAXINT)
  ui_range (1, 16)

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE tiff-load.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <tiffio.h>

typedef enum {
  TIFF_LOADING_RGBA,
  TIFF_LOADING_CONTIGUOUS,
  TIFF_LOADING_SEPARATED
} LoadingMode;

typedef struct
{
  GFile *file;
  GInputStream *stream;
  gboolean can_seek;

  gchar *buffer;
  gsize allocated;
  gsize position;
  gsize loaded;

  TIFF *tiff;

  gint directory;

  const Babl *format;
  LoadingMode mode;

  gint width;
  gint height;
} Priv;

static void
cleanup(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      if (p->tiff != NULL)
        TIFFClose(p->tiff);
      else if (p->stream != NULL)
        g_input_stream_close(G_INPUT_STREAM(p->stream), NULL, NULL);
      if (p->stream != NULL)
        g_clear_object(&p->stream);
      p->tiff = NULL;

      if (p->file != NULL)
        g_clear_object(&p->file);

      p->width = p->height = 0;
      p->directory = 0;
    }
}

static GSeekType
lseek_to_seek_type(int whence)
{
  switch (whence)
    {
    default:
    case SEEK_SET:
      return G_SEEK_SET;

    case SEEK_CUR:
      return G_SEEK_CUR;

    case G_SEEK_END:
      return SEEK_END;
    }
}

static tsize_t
read_from_stream(thandle_t handle,
                 tdata_t buffer,
                 tsize_t size)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gchar *new_buffer;
  gsize new_size = 1;
  gsize missing, needed;
  gssize read = -1;

  g_assert(p->stream);

  if (p->can_seek)
    {
      read = g_input_stream_read(G_INPUT_STREAM(p->stream),
                                 (void *) buffer, (gsize) size,
                                 NULL, &error);
      if (read < 0)
        {
          g_warning(error->message);
          g_error_free(error);
        }
    }
  else
    {
      if (p->position + size > p->loaded)
        {
          missing = p->position + size - p->loaded;
          if (p->loaded + missing > p->allocated)
            {
              needed = p->loaded + missing - p->allocated;
              while (new_size < p->allocated + needed)
                new_size *= 2;

              new_buffer = g_try_realloc(p->buffer, new_size);
              if (!new_buffer)
                return -1;

              p->allocated = new_size;
              p->buffer = new_buffer;
            }

          while (missing > 0)
            {
              read = g_input_stream_read(G_INPUT_STREAM(p->stream),
                                         (void *) (p->buffer + p->loaded),
                                         missing,
                                         NULL, &error);
              if (read < 0)
                {
                  g_warning(error->message);
                  g_error_free(error);
                  break;
                }

              p->loaded += read;
              missing -= read;
            }
        }

      g_assert(p->position + size <= p->loaded);

      memcpy(buffer, p->buffer + p->position, size);
      p->position += size;
      read = size;
    }

  return (tsize_t) read;
}

static tsize_t
write_to_stream(thandle_t handle,
                tdata_t buffer,
                tsize_t size)
{
  Priv *p = (Priv*) handle;

  g_assert(p->stream && FALSE);

  return -1;
}

static toff_t
seek_in_stream(thandle_t handle,
               toff_t offset,
               int whence)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gboolean sought = FALSE;
  goffset position = -1;

  g_assert(p->stream);

  if (p->can_seek)
    {
      sought = g_seekable_seek(G_SEEKABLE(p->stream),
                               (goffset) offset, lseek_to_seek_type(whence),
                               NULL, &error);
      if (sought)
        position = g_seekable_tell(G_SEEKABLE(p->stream));
      else
        {
          g_warning(error->message);
          g_error_free(error);
        }
    }
  else
    {
      switch (whence)
        {
        default:
        case SEEK_SET:
          if (offset <= p->loaded)
            position = p->position = offset;
          break;

        case SEEK_CUR:
          if (p->position + offset <= p->loaded)
            position = p->position += offset;
          break;

        case G_SEEK_END:
          if (p->loaded + offset <= p->loaded)
            position = p->position = p->loaded + offset;
          break;
        }
    }

  return (toff_t) position;
}

static int
close_stream(thandle_t handle)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gboolean closed = FALSE;

  g_assert(p->stream);

  closed = g_input_stream_close(G_INPUT_STREAM(p->stream),
                                NULL, &error);
  if (!closed)
    {
      g_warning(error->message);
      g_error_free(error);
    }

  g_clear_object(&p->stream);

  p->loaded = 0;
  p->position = 0;

  if (p->buffer != NULL)
    g_free(p->buffer);
  p->buffer = NULL;

  p->allocated = 0;

  return (closed) ? 0 : -1;
}

static toff_t
get_file_size(thandle_t handle)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  GFileInfo *info;
  goffset size;

  g_assert(p->stream);

  size = p->loaded;

  if (p->file != NULL)
    {
      info = g_file_query_info(p->file,
                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
                               G_FILE_QUERY_INFO_NONE,
                               NULL, &error);
      if (info == NULL)
        {
          g_warning(error->message);
          g_error_free(error);
        }
      else
        {
          if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_STANDARD_SIZE))
            size = g_file_info_get_size(info);
          g_object_unref(info);
        }
    }

  return (toff_t) size;
}

static gint
query_tiff(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  gshort color_space, compression;
  gushort bits_per_sample, samples_per_pixel;
  gushort sample_format;
  gboolean has_alpha = FALSE;
  gboolean alpha_is_premultiplied = FALSE;
  gushort *extra_types = NULL;
  gushort nb_extras, planar_config;
  gboolean fallback_mode = FALSE;
  gchar format_string[32];
  guint width, height;

  g_return_val_if_fail(p->tiff != NULL, -1);

  if (!TIFFGetField(p->tiff, TIFFTAG_IMAGEWIDTH, &width))
    {
      g_warning("could not get TIFF image width");
      return -1;
    }
  else if (!TIFFGetField(p->tiff, TIFFTAG_IMAGELENGTH, &height))
    {
      g_warning("could not get TIFF image height");
      return -1;
    }

  TIFFGetFieldDefaulted(p->tiff, TIFFTAG_COMPRESSION, &compression);
  if (!TIFFGetField(p->tiff, TIFFTAG_PHOTOMETRIC, &color_space))
    {
      g_warning("could not get photometric from TIFF image");
      if (compression == COMPRESSION_CCITTFAX3 ||
          compression == COMPRESSION_CCITTFAX4 ||
          compression == COMPRESSION_CCITTRLE  ||
          compression == COMPRESSION_CCITTRLEW)
        {
          g_message("assuming min-is-white (CCITT compressed)");
          color_space = PHOTOMETRIC_MINISWHITE;
        }
      else
        {
          g_message("assuming min-is-black");
          color_space = PHOTOMETRIC_MINISBLACK;
        }
    }

  TIFFGetFieldDefaulted(p->tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
  if (!TIFFGetField(p->tiff, TIFFTAG_EXTRASAMPLES, &nb_extras, &extra_types))
    nb_extras = 0;

  if (nb_extras > 0)
    {
      if (extra_types[0] == EXTRASAMPLE_ASSOCALPHA)
        {
          has_alpha = TRUE;
          alpha_is_premultiplied = TRUE;
          nb_extras--;
        }
      else if (extra_types[0] == EXTRASAMPLE_UNASSALPHA)
        {
          has_alpha = TRUE;
          alpha_is_premultiplied = FALSE;
          nb_extras--;
        }
      else if (extra_types[0] == EXTRASAMPLE_UNSPECIFIED)
        {
          has_alpha = TRUE;
          alpha_is_premultiplied = FALSE;
          nb_extras--;
        }
    }

  switch(color_space)
    {
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_MINISWHITE:
      if (samples_per_pixel > 1 + nb_extras)
        {
          nb_extras = samples_per_pixel - 2;
          has_alpha = TRUE;
        }

      if (has_alpha)
        {
          if(alpha_is_premultiplied)
            g_strlcpy(format_string, "Y'aA ", 32);
          else
            g_strlcpy(format_string, "Y'A ", 32);
        }
      else
        g_strlcpy(format_string, "Y' ", 32);
      break;

    case PHOTOMETRIC_RGB:
      if (samples_per_pixel > 3 + nb_extras)
        {
          nb_extras = samples_per_pixel - 4;
          has_alpha = TRUE;
        }

      if (has_alpha)
        {
          if (alpha_is_premultiplied)
            g_strlcpy(format_string, "R'aG'aB'aA ", 32);
          else
            g_strlcpy(format_string, "R'G'B'A ", 32);
        }
      else
        g_strlcpy(format_string, "R'G'B' ", 32);
      break;

    default:
      fallback_mode = TRUE;
      break;
    }

  TIFFGetFieldDefaulted(p->tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);
  TIFFGetFieldDefaulted(p->tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);

  switch(bits_per_sample)
    {
    case 8:
      g_strlcat(format_string, "u8", 32);
      break;

    case 16:
      if (sample_format == SAMPLEFORMAT_IEEEFP)
        g_strlcat(format_string, "half", 32);
      else
        g_strlcat(format_string, "u16", 32);
      break;

    case 32:
      if (sample_format == SAMPLEFORMAT_IEEEFP)
        g_strlcat(format_string, "float", 32);
      else
        g_strlcat(format_string, "u32", 32);
      break;

    case 64:
      g_strlcat(format_string, "double", 32);
      break;

    default:
      fallback_mode = TRUE;
      break;
    }

  if (fallback_mode == TRUE)
    g_strlcpy(format_string, "R'aG'aB'aA u8", 32);

  TIFFGetFieldDefaulted(p->tiff, TIFFTAG_PLANARCONFIG, &planar_config);

  p->format = babl_format(format_string);
  if (fallback_mode)
    p->mode = TIFF_LOADING_RGBA;
  else if (planar_config == PLANARCONFIG_CONTIG)
    p->mode = TIFF_LOADING_CONTIGUOUS;
  else
    p->mode = TIFF_LOADING_SEPARATED;

  p->height = (gint) height;
  p->width = (gint) width;

  return 0;
}

static gint
load_RGBA(GeglOperation *operation,
          GeglBuffer    *output)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  guint32 *buffer;
  gint row;

  g_return_val_if_fail(p->tiff != NULL, -1);

  buffer = g_try_new(guint32, p->width * p->height * sizeof(guint32));

  g_assert(buffer != NULL);

  if (!TIFFReadRGBAImage(p->tiff, p->width, p->height, buffer, 0))
    {
      g_message("unsupported layout, RGBA loader failed");
      g_free(buffer);
      return -1;
    }

  for (row = 0; row < p->height; row++)
    {
      GeglRectangle line = { 0, p->height - row - 1, p->width, 1 };
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
      guint row_start = row * p->width;
      guint row end = row * p->width + p->width;
      guint i;

      for (i = row_start; i < row_end; i++)
        buffer[i] = GUINT32_TO_LE(buffer[i]);
#endif

      gegl_buffer_set(output, &line, 0, p->format,
                      ((guchar *) buffer) + (row * p->width * 4),
                      GEGL_AUTO_ROWSTRIDE);
    }

  g_free(buffer);
  return 0;
}

static gint
load_contiguous(GeglOperation *operation,
                GeglBuffer    *output)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  guint32 tile_width = (guint32) p->width;
  guint32 tile_height = 1;
  guchar *buffer;
  gint x, y;

  g_return_val_if_fail(p->tiff != NULL, -1);

  if (!TIFFIsTiled(p->tiff))
      buffer = g_try_new(guchar, TIFFScanlineSize(p->tiff));
  else
    {
      TIFFGetField(p->tiff, TIFFTAG_TILEWIDTH, &tile_width);
      TIFFGetField(p->tiff, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_try_new(guchar, TIFFTileSize(p->tiff));
    }

  g_assert(buffer != NULL);

  for (y = 0; y < p->height; y += tile_height)
    {
      for (x = 0; x < p->width; x += tile_width)
        {
          GeglRectangle tile = { x, y, tile_width, tile_height };

          if (TIFFIsTiled(p->tiff))
            TIFFReadTile(p->tiff, buffer, x, y, 0, 0);
          else
            TIFFReadScanline(p->tiff, buffer, y, 0);

          gegl_buffer_set(output, &tile, 0, p->format,
                          (guchar *) buffer,
                          GEGL_AUTO_ROWSTRIDE);
        }
    }

  g_free(buffer);
  return 0;
}

static gint
load_separated(GeglOperation *operation,
               GeglBuffer    *output)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  guint32 tile_width = (guint32) p->width;
  guint32 tile_height = 1;
  gint output_bytes_per_pixel;
  gint nb_components, offset = 0;
  guchar *buffer;
  gint i;

  g_return_val_if_fail(p->tiff != NULL, -1);

  if (!TIFFIsTiled(p->tiff))
    buffer = g_try_new(guchar, TIFFScanlineSize(p->tiff));
  else
    {
      TIFFGetField(p->tiff, TIFFTAG_TILEWIDTH, &tile_width);
      TIFFGetField(p->tiff, TIFFTAG_TILELENGTH, &tile_height);

      buffer = g_try_new(guchar, TIFFTileSize(p->tiff));
    }

  g_assert(buffer != NULL);

  nb_components = babl_format_get_n_components(p->format);
  output_bytes_per_pixel = babl_format_get_bytes_per_pixel(p->format);

  for (i = 0; i < nb_components; i++)
    {
      const Babl *plane_format;
      const Babl *component_type;
      gint plane_bytes_per_pixel;
      gint x, y;

      component_type = babl_format_get_type(p->format, i);

      plane_format = babl_format_n(component_type, 1);

      plane_bytes_per_pixel = babl_format_get_bytes_per_pixel(plane_format);

      for (y = 0; y < p->height; y += tile_height)
        {
          for (x = 0; x < p->width; x += tile_width)
            {
              GeglRectangle output_tile = { x, y, tile_width, tile_height };
              GeglRectangle plane_tile = { 0, 0, tile_width, tile_height };
              GeglBufferIterator *iterator;
              GeglBuffer *linear;

              if (TIFFIsTiled(p->tiff))
                TIFFReadTile(p->tiff, buffer, x, y, 0, i);
              else
                TIFFReadScanline(p->tiff, buffer, y, i);

              linear = gegl_buffer_linear_new_from_data(buffer, plane_format,
                                                        &plane_tile,
                                                        GEGL_AUTO_ROWSTRIDE,
                                                        NULL, NULL);

              iterator = gegl_buffer_iterator_new(linear, &plane_tile,
                                                  0, NULL,
                                                  GEGL_ACCESS_READ,
                                                  GEGL_ABYSS_NONE);

              gegl_buffer_iterator_add(iterator, output, &output_tile,
                                       0, p->format,
                                       GEGL_ACCESS_READWRITE,
                                       GEGL_ABYSS_NONE);

              while (gegl_buffer_iterator_next(iterator))
                {
                  guchar *plane_buffer = iterator->data[0];
                  guchar *output_buffer = iterator->data[1];
                  gint nb_pixels = iterator->length;

                  output_buffer += offset;

                  while (nb_pixels--)
                  {
                    memcpy(output_buffer, plane_buffer, plane_bytes_per_pixel);

                    output_buffer += output_bytes_per_pixel;
                    plane_buffer += plane_bytes_per_pixel;
                  }
                }

              g_object_unref(linear);
            }
        }

      offset += plane_bytes_per_pixel;
    }

  g_free(buffer);
  return 0;
}

static void
prepare(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (o->user_data) ? o->user_data : g_new0(Priv, 1);
  GError *error = NULL;
  GFile *file = NULL;
  gint directories;

  g_assert(p != NULL);

  if (p->file != NULL && (o->uri || o->path))
    {
      if (o->uri && strlen(o->uri) > 0)
        file = g_file_new_for_uri(o->uri);
      else if (o->path && strlen(o->path) > 0)
        file = g_file_new_for_path(o->path);
      if (file != NULL)
        {
          if (!g_file_equal(p->file, file))
            cleanup(operation);
          g_object_unref(file);
        }
    }

  o->user_data = (void*) p;

  if (p->stream == NULL)
    {
      p->stream = gegl_gio_open_input_stream(o->uri, o->path, &p->file, &error);
      if (p->stream != NULL && p->file != NULL)
        p->can_seek = g_seekable_can_seek(G_SEEKABLE(p->stream));
      if (p->stream == NULL)
        {
          g_warning(error->message);
          g_error_free(error);
          cleanup(operation);
          return;
        }

      p->tiff = TIFFClientOpen("GEGL-tiff-load", "r", (thandle_t) p,
                               read_from_stream, write_to_stream,
                               seek_in_stream, close_stream,
                               get_file_size, NULL, NULL);
      if (p->tiff == NULL)
        {
          g_warning("failed to open TIFF from %s", o->path);
          g_input_stream_close(p->stream, NULL, NULL);
          if (p->file != NULL)
            g_object_unref(p->file);
          cleanup(operation);
          return;
        }
    }

  if (o->directory != p->directory)
    {
      directories = TIFFNumberOfDirectories(p->tiff);
      if (o->directory > 1 && o->directory <= directories)
        TIFFSetDirectory(p->tiff, o->directory - 1);

      if (query_tiff(operation))
        {
          g_warning("could not query TIFF file");
          cleanup(operation);
          return;
        }

        p->directory = o->directory;
    }

  gegl_operation_set_format(operation, "output", p->format);
}

static GeglRectangle
get_bounding_box(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  Priv *p = (Priv*) o->user_data;

  if (p->tiff != NULL)
    {
      result.width = p->width;
      result.height = p->height;
    }

  return result;
}

static gboolean
process(GeglOperation *operation,
        GeglBuffer *output,
        const GeglRectangle *result,
        gint level)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;

  if (p->tiff != NULL)
    {
      switch (p->mode)
      {
      case TIFF_LOADING_RGBA:
        if (!load_RGBA(operation, output))
          return TRUE;
        break;

      case TIFF_LOADING_CONTIGUOUS:
        if (!load_contiguous(operation, output))
          return TRUE;
        break;

      case TIFF_LOADING_SEPARATED:
        if (!load_separated(operation, output))
          return TRUE;
        break;

      default:
        break;
      }
    }

  return FALSE;
}

static GeglRectangle
get_cached_region(GeglOperation       *operation,
                  const GeglRectangle *roi)
{
  return get_bounding_box(operation);
}

static void
finalize(GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES(object);

  if (o->user_data != NULL)
    {
      cleanup(GEGL_OPERATION(object));
      if (o->user_data != NULL)
        g_free(o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS(gegl_op_parent_class)->finalize(object);
}

static void
gegl_op_class_init(GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS(klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS(klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS(klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys(operation_class,
    "name",          "gegl:tiff-load",
    "title",       _("TIFF File Loader"),
    "categories",    "hidden",
    "description", _("TIFF image loader using libtiff"),
    NULL);

  gegl_extension_handler_register(".tiff", "gegl:tiff-load");
  gegl_extension_handler_register(".tif", "gegl:tiff-load");
}

#endif
