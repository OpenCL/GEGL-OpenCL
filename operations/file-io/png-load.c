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
 *           2006 Dominik Ernst <dernst@gmx.de>
 *           2006 Kevin Cozens <kcozens@cvs.gnome.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_path (path, "/tmp/romedalen.png", "Path of file to load.")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            png_load
#define GEGL_CHANT_DESCRIPTION     "PNG image loader."
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
                        gint        *ret_height,
                        gpointer     format);

gint query_png (const gchar *path,
                gint        *width,
                gint        *height,
                gpointer    *format);

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglChantOperation       *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer   *output = NULL;
  gint          result;
  gpointer      format;

    {
    gint width, height;

    if (strcmp(self->path, "-"))
      {
        result = query_png (self->path, &width, &height, &format);
        if (result)
          {
            g_warning ("%s is %s really a PNG file?",
              G_OBJECT_TYPE_NAME (operation), self->path);
            output = g_object_new (GEGL_TYPE_BUFFER,
                                   "format", babl_format ("R'G'B'A u8"),
                                   "x",      0,
                                   "y",      0,
                                   "width",  10,
                                   "height", 10,
                                   NULL);
            gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (output));
            return TRUE;
          }
      }

    output = g_object_new (GEGL_TYPE_BUFFER,
                           "format", format,
                           "x",      0,
                           "y",      0,
                           "width",  width,
                           "height", height,
                           NULL);

    result = gegl_buffer_import_png (output, self->path, 0, 0,
                                     &width, &height, format);

    if (result)
      {
        g_warning ("%s failed to open file %s for reading.",
          G_OBJECT_TYPE_NAME (operation), self->path);
        if (output)
          {
            g_object_unref (output);

          }
        output = g_object_new (GEGL_TYPE_BUFFER,
                               "format", format,
                               "x",      0,
                               "y",      0,
                               "width",  10,
                               "height", 10,
                               NULL);

        gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (output));
        return TRUE;
      }
    }
  gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (output));
  return  TRUE;
}


static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglChantOperation       *self = GEGL_CHANT_OPERATION (operation);
  /*GeglOperationSource *source = GEGL_OPERATION_SOURCE (operation);*/
  gint width, height;
  gint status;
  gpointer format;

  /*if (!strcmp (self->path, "-"))
    {
      process (operation, dynamic_id);
      width = source->output->width;
      height = source->output->height;
    }
  else*/
    {
      status = query_png (self->path, &width, &height, &format);

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
                        gint        *ret_height,
                        gpointer     format)
{
  gint           width;
  gint           height;
  gint           bit_depth;
  gint           bpp;
  gint           number_of_passes=1;
  png_uint_32    w;
  png_uint_32    h;
  FILE          *infile;
  png_structp    load_png_ptr;
  png_infop      load_info_ptr;
  unsigned char  header[8];
  guchar        *pixels;
  //png_bytep     *rows;


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
    height = h;
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
          fclose (infile);
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

    if (load_info_ptr->valid & PNG_INFO_gAMA)
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
    GeglBuffer    *tmp_buf;
    GeglRectangle  rect;

    tmp_buf = g_object_new( GEGL_TYPE_BUFFER,
                            "source", gegl_buffer,
                            "x",      dest_x,
                            "y",      dest_y,
                            "width",  width,
                            "height", height,
                            NULL);

    for (pass=0; pass<number_of_passes; pass++)
      {
        for(i=0; i<h; i++)
          {
            gegl_rect_set (&rect, 0, i, width, 1);

            if (pass != 0)
              gegl_buffer_get (tmp_buf, &rect, pixels, format, 1.0);

            png_read_rows (load_png_ptr, &pixels, NULL, 1);
            gegl_buffer_set (tmp_buf, &rect, pixels, format);
          }
      }

      g_object_unref (tmp_buf);
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
                gint        *height,
                gpointer    *format)
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
    gchar format_string[32];

    png_get_IHDR (load_png_ptr,
                  load_info_ptr,
                  &w, &h,
                  &bit_depth,
                  &color_type,
                  NULL, NULL, NULL);
    *width = w;
    *height = h;

    if (load_info_ptr->valid & PNG_INFO_tRNS)
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
        fclose (infile);
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
        fclose (infile);
        return -1;
      }

    *format = babl_format (format_string);
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
