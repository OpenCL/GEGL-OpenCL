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
 * Copyright 2008 Hubert Figuière <hub@figuiere.net>
 */


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, "File", "", "Path of file to load.")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "openraw.c"

#include "config.h"
#include "gegl-chant.h"
#include <stdio.h>
#include <libopenraw/libopenraw.h>

static gint
query_raw (const gchar *path,
           gint        *width,
           gint        *height)
{
  or_error err;
  uint32_t x, y;
  ORRawDataRef rawdata;
  or_data_type raw_format = OR_DATA_TYPE_NONE;
  ORRawFileRef rawfile = or_rawfile_new(path, OR_RAWFILE_TYPE_UNKNOWN);

  if (!rawfile) {
    return 1;
  }

  rawdata = or_rawdata_new();
  err = or_rawfile_get_rawdata(rawfile, rawdata, OR_OPTIONS_NONE);
  if(err == OR_ERROR_NONE) {
    raw_format = or_rawdata_format(rawdata);
    if(raw_format == OR_DATA_TYPE_CFA) {
      or_rawdata_dimensions(rawdata, &x, &y);
      *width = x;
      *height = y;
    }
  }

  or_rawdata_release(rawdata);
  or_rawfile_release(rawfile);
  return ((err != OR_ERROR_NONE) || (raw_format != OR_DATA_TYPE_CFA));
}


static gint
gegl_buffer_import_raw (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         dest_x,
                        gint         dest_y)
{
  ORRawDataRef rawdata;
  or_data_type raw_format = OR_DATA_TYPE_NONE;
  ORRawFileRef rawfile = or_rawfile_new(path, OR_RAWFILE_TYPE_UNKNOWN);
  or_error err;

  if (!rawfile) {
    return 1;
  }

  rawdata = or_rawdata_new();
  err = or_rawfile_get_rawdata(rawfile, rawdata, OR_OPTIONS_NONE);

  if(err != OR_ERROR_NONE) {
    uint32_t x, y;
    void *data;

    raw_format = or_rawdata_format(rawdata);
    if(raw_format == OR_DATA_TYPE_CFA) {
      /* TODO take the dest_x and dest_y into account */
      GeglRectangle rect = {0, 0, 0, 0};
      or_rawdata_dimensions(rawdata, &x, &y);
      rect.width = x;
      rect.height = y;

      data = or_rawdata_data(rawdata);
      gegl_buffer_set(gegl_buffer, &rect, babl_format ("Y u16"),
                      data, GEGL_AUTO_ROWSTRIDE);
    }
  }
  or_rawdata_release(rawdata);
  or_rawfile_release(rawfile);
  return ((err != OR_ERROR_NONE) || (raw_format != OR_DATA_TYPE_CFA));
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  gint width, height;
  gint status;
  gegl_operation_set_format (operation, "output", babl_format ("Y u16"));
  status = query_raw (o->path, &width, &height);

  if (status)
    {
      /*g_warning ("calc have rect of %s failed", o->path);*/
      result.width  = 10;
      result.height  = 10;
    }
  else
    {
      result.width  = width;
      result.height  = height;
    }

  return result;
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle        rect={0,0};
  gint                 problem;

  problem = query_raw (o->path, &rect.width, &rect.height);

  if (problem)
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }


  problem = gegl_buffer_import_raw (output, o->path, 0, 0);

  if (problem)
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);

      return FALSE;
    }

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  static gboolean done = FALSE;

  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->name        = "gegl:openraw-load";
  operation_class->categories  = "hidden";
  operation_class->description = "Camera RAW image loader";

  if (done)
    return;

  /* query libopenraw instead. need a new API */
  gegl_extension_handler_register (".cr2", "gegl:openraw-load");
  gegl_extension_handler_register (".CR2", "gegl:openraw-load");
  gegl_extension_handler_register (".crw", "gegl:openraw-load");
  gegl_extension_handler_register (".CRW", "gegl:openraw-load");
  gegl_extension_handler_register (".erf", "gegl:openraw-load");
  gegl_extension_handler_register (".ERF", "gegl:openraw-load");
  gegl_extension_handler_register (".mrw", "gegl:openraw-load");
  gegl_extension_handler_register (".MRW", "gegl:openraw-load");
  gegl_extension_handler_register (".nef", "gegl:openraw-load");
  gegl_extension_handler_register (".NEF", "gegl:openraw-load");
  gegl_extension_handler_register (".dng", "gegl:openraw-load");
  gegl_extension_handler_register (".DNG", "gegl:openraw-load");

  done = TRUE;
}

#endif
