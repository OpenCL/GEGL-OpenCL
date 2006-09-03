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
 
gegl_chant_string (path, "/tmp/romedalen.png", "path to file to load")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            png_load
#define GEGL_CHANT_DESCRIPTION     "loads a png file using libpng, currently restricted to 8bpc"
#define GEGL_CHANT_SELF            "png-load.c"
#define GEGL_CHANT_CATEGORIES      "hidden"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <png.h>

static gint
gegl_buffer_import_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         dest_x,
                        gint         dest_y,
                        gint        *ret_width,
                        gint        *ret_height);

gint query_png (const gchar *path,
                gint        *width,
                gint        *height);

static gboolean
process (GeglOperation *operation,
          const gchar   *output_prop)
{
  ChantInstance       *self = GEGL_CHANT_INSTANCE (operation);
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  gint          result;

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

    {
    gint width, height;

    if (strcmp(self->path, "-"))
      {
        result = query_png (self->path, &width, &height);
        if (result)
          {
            g_warning ("%s is %s really a PNG file?",
              G_OBJECT_TYPE_NAME (operation), self->path);
        op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                      "format", babl_format ("R'G'B'A u8"),
                                      "x",      0,
                                      "y",      0,
                                      "width",  10,
                                      "height", 10,
                                      NULL);
            return TRUE;
          }
      }

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                      "format", babl_format ("R'G'B'A u8"),
                                      "x",      0,
                                      "y",      0,
                                      "width",  width,
                                      "height", height,
                                      NULL);

    result = gegl_buffer_import_png (op_source->output, self->path, 0, 0,
                                     &width, &height);

    if (result)
      {
        g_warning ("%s failed to open file %s for reading.",
          G_OBJECT_TYPE_NAME (operation), self->path);
        if (op_source->output)
          {
            g_object_unref (op_source->output);
            op_source->output = NULL;
          }
        op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                      "format", babl_format ("R'G'B'A u8"),
                                      "x",      0,
                                      "y",      0,
                                      "width",  10,
                                      "height", 10,
                                      NULL);
        return TRUE;
      }
    }
  return  TRUE;
}


static GeglRect 
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {0,0,0,0};
  ChantInstance       *self = GEGL_CHANT_INSTANCE (operation);
  GeglOperationSource *source = GEGL_OPERATION_SOURCE (operation);
  gint width, height;
  gint status;

  if (!strcmp (self->path, "-"))
    {
      process (operation, "output");
      width = source->output->width;
      height = source->output->height;
    }
  else
    {
      status = query_png (self->path, &width, &height);

      if (status)
        {
          g_warning ("calc have rect of %s failed", self->path);
        }
    }
  result.w = width;
  result.h = height;
  return result;
}

static gint
gegl_buffer_import_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         dest_x,
                        gint         dest_y,
                        gint        *ret_width,
                        gint        *ret_height)
{
  gint           width;
  gint           height;
  gint           row_stride;
  png_uint_32    w;
  png_uint_32    h;
  FILE          *infile;
  png_structp    load_png_ptr;
  png_infop      load_info_ptr;
  unsigned char  header[8];
  guchar        *pixels;

  unsigned   int i;
  png_bytep  *row_p = NULL;

  if (!strcmp (path, "-"))
    {
      infile = stdin;
    }
  else
    {
      infile = fopen (path, "rb");
    }
  if (!infile)
    {
      return -1;
    }

  fread (header, 1, 8, infile);
  if (png_sig_cmp (header, 0, 8))
    {
      fclose (infile);
      g_warning ("%s is not a png file", path);
      return -1;
    }

  load_png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!load_png_ptr)
    {
      fclose (infile);
      return -1;
    }

  load_info_ptr = png_create_info_struct (load_png_ptr);
  if (!load_info_ptr)
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
      fclose (infile);
      return -1;
    }

  if (setjmp (png_jmpbuf (load_png_ptr)))
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
     if (row_p)
        g_free (row_p);
      fclose (infile);
      return -1;
    }

  png_init_io (load_png_ptr, infile);
  png_set_sig_bytes (load_png_ptr, 8);
  png_read_info (load_png_ptr, load_info_ptr);
  {
    int bit_depth;
    int color_type;

    png_get_IHDR (load_png_ptr, load_info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);
    width = w;
    height = h;
    if (ret_width)
      *ret_width = w;
    if (ret_height)
      *ret_height = h;
    png_set_strip_16 (load_png_ptr);
    if ((color_type == PNG_COLOR_TYPE_RGB))
      {
	png_set_add_alpha (load_png_ptr, 0xffffffff, 1);
      }
    else if (!(color_type == PNG_COLOR_TYPE_RGBA))
      {
        g_warning ("bpp or type mismatch");
        png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
        fclose (infile);
        return -1;
      }

    if (png_get_interlace_type (load_png_ptr, load_info_ptr) == PNG_INTERLACE_ADAM7)
      {
        g_warning ("not supporting interlaced pngs");
        png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
        fclose (infile);
        return -1;
      }
  }

  row_stride = w * 4;
  pixels = g_malloc0 (row_stride);

  for (i=0; i< h; i++)
    {
      GeglBuffer *rect = g_object_new (GEGL_TYPE_BUFFER,
                    "source", gegl_buffer,
                    "x",      dest_x,
                    "y",      dest_y + i,
                    "width",  width,
                    "height", 1,
                    NULL);
      png_read_rows (load_png_ptr, &pixels, NULL, 1);
      gegl_buffer_set_fmt (rect, pixels, babl_format ("R'G'B'A u8"));
      g_object_unref (rect);
    }

  png_read_end (load_png_ptr, NULL);
  png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);

  g_free (pixels);
  if (infile!=stdin)
    fclose (infile);

  return 0;
}

gint query_png (const gchar *path,
                gint        *width,
                gint        *height)
{
  png_uint_32   w;
  png_uint_32   h;
  FILE         *infile;
  png_structp   load_png_ptr;
  png_infop     load_info_ptr;
  unsigned char header[8];

  png_bytep  *row_p = NULL;

  infile = fopen (path, "rb");
  if (!infile)
    {
      return -1;
    }

  fread (header, 1, 8, infile);
  if (png_sig_cmp (header, 0, 8))
    {
      fclose (infile);
      g_warning ("%s is not a png file", path);
      return -1;
    }

  load_png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!load_png_ptr)
    {
      fclose (infile);
      return -1;
    }

  load_info_ptr = png_create_info_struct (load_png_ptr);
  if (!load_info_ptr)
    {
      png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
      fclose (infile);
      return -1;
    }

  if (setjmp (png_jmpbuf (load_png_ptr)))
    {
     png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
     if (row_p)
        g_free (row_p);
      fclose (infile);
      return -1;
    }

  png_init_io (load_png_ptr, infile);
  png_set_sig_bytes (load_png_ptr, 8);
  png_read_info (load_png_ptr, load_info_ptr);
  {
    int bit_depth;
    int color_type;

    png_get_IHDR (load_png_ptr, load_info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);
    *width = w;
    *height = h;
    png_set_strip_16 (load_png_ptr);
    if ((color_type == PNG_COLOR_TYPE_RGB))
      {
	png_set_add_alpha (load_png_ptr, 0xffffffff, 1);
      }
    else if (!(color_type == PNG_COLOR_TYPE_RGBA))
      {
        g_warning ("color type mismatch");
        png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
        fclose (infile);
        return -1;
      }
  }
  png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);
  fclose (infile);
  return 0;
}

static void class_init (GeglOperationClass *operation_class)
{
  static gboolean done=FALSE;
  if (done)
    return;
  gegl_extension_handler_register (".png", "png-load");
  gegl_extension_handler_register (".PNG", "png-load");
  done = TRUE;
}

#endif
