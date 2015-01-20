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
    description(_("Path of file to load."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE webp-load.c

#include "gegl-op.h"
#include <webp/decode.h>

static gboolean
read_webp (const gchar *path, GeglBuffer *buf, GeglRectangle *bounds_out, const Babl **format_out)
{
  GMappedFile *map = g_mapped_file_new (path, FALSE, NULL);

  const gpointer data = g_mapped_file_get_contents (map);
  gsize data_size = g_mapped_file_get_length (map);

  const Babl* format;
  GeglRectangle bounds = {0, };

  WebPDecoderConfig config;
  if (!WebPInitDecoderConfig (&config) ||
      WebPGetFeatures (data, data_size, &config.input) != VP8_STATUS_OK)
    {
      g_mapped_file_unref (map);
      return FALSE;
    }

  bounds.width  = config.input.width;
  bounds.height = config.input.height;

  if (config.input.has_alpha)
    {
      config.output.colorspace = MODE_RGBA;
      format = babl_format ("R'G'B'A u8");
    }
  else
    {
      config.output.colorspace = MODE_RGB;
      format = babl_format ("R'G'B' u8");
    }

  if (buf)
    {
      if (WebPDecode (data, data_size, &config) != VP8_STATUS_OK)
        {
          WebPFreeDecBuffer (&config.output);
          g_mapped_file_unref (map);
          return FALSE;
        }

      gegl_buffer_set (buf, &bounds, 0, format, config.output.u.RGBA.rgba,
                       config.output.u.RGBA.stride);

      WebPFreeDecBuffer (&config.output);
    }

  if (bounds_out)
    *bounds_out = bounds;

  if (format_out)
    *format_out = format;

  g_mapped_file_unref (map);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  const Babl   *format = NULL;

  read_webp (o->path, NULL, &result, &format);

  if (format)
    gegl_operation_set_format (operation, "output", format);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  return read_webp (o->path, output, NULL, NULL);
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
    "name",         "gegl:webp-load",
    "title",        _("WebP File Loader"),
    "categories"  , "hidden",
    "description" , _("WebP image loader."),
    NULL);

  gegl_extension_handler_register (".webp", "gegl:webp-load");
}

#endif
