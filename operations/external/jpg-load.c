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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "jpg-load.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <jpeglib.h>

static gint
gegl_jpg_load_query_jpg (const gchar *path,
                         gint        *width,
                         gint        *height)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *infile;

  if ((infile = fopen (path, "rb")) == NULL)
    {
      /*g_warning ("unable to open %s for jpeg import", path);*/
      return -1;
    }

  jpeg_create_decompress (&cinfo);
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_stdio_src (&cinfo, infile);

  (void) jpeg_read_header (&cinfo, TRUE);
  (void) jpeg_start_decompress (&cinfo);

  if (cinfo.output_components != 3)
    {
      g_warning ("attempted to load non RGB JPEG");
      jpeg_destroy_decompress (&cinfo);
      return -1;
    }

  if (width)
    *width = cinfo.output_width;
  if (height)
    *height = cinfo.output_height;

  jpeg_destroy_decompress (&cinfo);

  fclose (infile);
  return 0;
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
  int row=0;

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

  if (cinfo.output_components != 3)
    {
      g_warning ("attempted to load non RGB JPEG");
      jpeg_destroy_decompress (&cinfo);
      return -1;
    }

  row_stride = cinfo.output_width * cinfo.output_components;

  if ((row_stride) % 2)
    (row_stride)++;

  /* allocated with the jpeg library, and freed with the decompress context */
  buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  while (cinfo.output_scanline < cinfo.output_height)
    {
      GeglRectangle rect;

      rect.x = dest_x;
      rect.y = dest_y + row++;
      rect.width = cinfo.output_width;
      rect.height = 1;

      jpeg_read_scanlines (&cinfo, buffer, 1);
      gegl_buffer_set (gegl_buffer, &rect, babl_format ("R'G'B' u8"), buffer[0],
                       GEGL_AUTO_ROWSTRIDE);
    }
  jpeg_destroy_decompress (&cinfo);
  fclose (infile);
  return 0;
}

static GeglRectangle
gegl_jpg_load_get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  gint width, height;
  gint status;
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u8"));
  status = gegl_jpg_load_query_jpg (o->path, &width, &height);

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
gegl_jpg_load_process (GeglOperation       *operation,
                       GeglBuffer          *output,
                       const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle        rect={0,0};
  gint                 problem;

  problem = gegl_jpg_load_query_jpg (o->path, &rect.width, &rect.height);

  if (problem)
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }


  problem = gegl_jpg_load_buffer_import_jpg (output, o->path, 0, 0);

  if (problem)
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
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = gegl_jpg_load_process;
  operation_class->get_bounding_box = gegl_jpg_load_get_bounding_box;
  operation_class->get_cached_region = gegl_jpg_load_get_cached_region;

  operation_class->name        = "gegl:jpg-load";
  operation_class->categories  = "hidden";
  operation_class->description = _("JPG image loader");

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_extension_handler_register (".jpg", "gegl:jpg-load");
  gegl_extension_handler_register (".JPG", "gegl:jpg-load");
  gegl_extension_handler_register (".jpeg", "gegl:jpg-load");
  gegl_extension_handler_register (".JPEG", "gegl:jpg-load");
/*  done = TRUE; */
}
#endif
