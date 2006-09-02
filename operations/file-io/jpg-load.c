/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#ifdef GEGL_CHANT_PROPERTIES
 
gegl_chant_string (path, "/tmp/romedalen.jpg", "Path to jpg file on disk to load")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            jpg_load
#define GEGL_CHANT_DESCRIPTION     "loads a jpeg file using libjpeg"
#define GEGL_CHANT_SELF            "jpg-load.c"
#define GEGL_CHANT_CATEGORIES      "hidden"
#define GEGL_CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"
#include <stdio.h>
#include <jpeglib.h>

static gint
gegl_buffer_import_jpg (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         dest_x,
                        gint         dest_y);

static gint
query_jpg (const gchar *path,
           gint        *width,
           gint        *height);

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE (operation);
  ChantInstance       *self      = GEGL_CHANT_INSTANCE (operation);
  gint           width;
  gint           height;
  gint           result;

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output!=NULL)
    {
      g_object_unref (op_source->output);
      op_source->output=NULL;
    }

    {
      result = query_jpg (self->path, &width, &height);
      if (result)
        {
          g_warning ("%s failed to open file %s for reading.",
            G_OBJECT_TYPE_NAME (operation), self->path);
          op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                            "format", babl_format ("R'G'B' u8"),
                                            "x",      0,
                                            "y",      0,
                                            "width",  10,
                                            "height", 10,
                                            NULL);
              return TRUE;
        }

      op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                        "format", babl_format ("R'G'B' u8"),
                                        "x",      0,
                                        "y",      0,
                                        "width",  width,
                                        "height", height,
                                        NULL);

      result = gegl_buffer_import_jpg (op_source->output, self->path, 0, 0);

      if (result)
        {
          g_warning ("%s failed to open file %s for reading.",
            G_OBJECT_TYPE_NAME (operation), self->path);
          op_source->output = NULL;
          return FALSE;
        }
    }
  return  TRUE;
}


static gboolean
calc_have_rect (GeglOperation *operation)
{
  ChantInstance       *self      = GEGL_CHANT_INSTANCE (operation);
  gint width, height;
  gint status;
  status = query_jpg (self->path, &width, &height);

  if (status)
    {
      g_warning ("calc have rect of %s failed", self->path);
      gegl_operation_set_have_rect (operation, 0, 0, 10, 10);
      return TRUE;
    }

  gegl_operation_set_have_rect (operation, 0, 0, width, height);

  return TRUE;
}

static gint
query_jpg (const gchar *path,
           gint        *width,
           gint        *height)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *infile;

  if ((infile = fopen (path, "r")) == NULL)
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

  if (width)
    *width = cinfo.output_width;
  if (height)
    *height = cinfo.output_height;

  jpeg_destroy_decompress (&cinfo);

  fclose (infile);
  return 0;
}

static gint
gegl_buffer_import_jpg (GeglBuffer  *gegl_buffer,
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

  if ((infile = fopen (path, "r")) == NULL)
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
      GeglBuffer *rect = g_object_new (GEGL_TYPE_BUFFER,
                                       "source", gegl_buffer,
                                       "x", dest_x,
                                       "y", dest_y + row++,
                                       "width", cinfo.output_width,
                                       "height", 1,
                                       NULL);

      jpeg_read_scanlines (&cinfo, buffer, 1);
      gegl_buffer_set_fmt (rect, buffer[0], babl_format ("R'G'B' u8"));
      g_object_unref (rect);
    }
  jpeg_destroy_decompress (&cinfo);
  fclose (infile);
  return 0;
}

static void class_init (GeglOperationClass *operation_class)
{
  static gboolean done=FALSE;
  if (done)
    return;
  gegl_extension_handler_register (".jpg", "jpg-load");
  gegl_extension_handler_register (".JPG", "jpg-load");
  gegl_extension_handler_register (".jpeg", "jpg-load");
  gegl_extension_handler_register (".JPEG", "jpg-load");
  done = TRUE;
}

#endif
