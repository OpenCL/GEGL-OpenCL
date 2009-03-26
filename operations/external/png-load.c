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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "png-load.c"

#include "gegl-chant.h"
#include <png.h>

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
  /*png_bytep     *rows;*/


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
    GeglRectangle  rect;

    for (pass=0; pass<number_of_passes; pass++)
      {
        for(i=0; i<h; i++)
          {
            gegl_rectangle_set (&rect, 0, i, width, 1);

            if (pass != 0)
              gegl_buffer_get (gegl_buffer, 1.0, &rect, format, pixels, GEGL_AUTO_ROWSTRIDE);

            png_read_rows (load_png_ptr, &pixels, NULL, 1);
            gegl_buffer_set (gegl_buffer, &rect, format, pixels,
                             GEGL_AUTO_ROWSTRIDE);
          }
      }
  }


  png_read_end (load_png_ptr, NULL);
  png_destroy_read_struct (&load_png_ptr, &load_info_ptr, NULL);

  g_free (pixels);

  if (infile!=stdin)
    fclose (infile);

  return 0;
}

static gint query_png (const gchar *path,
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

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  gint          width, height;
  gint          status;
  gpointer      format;

  status = query_png (o->path, &width, &height, &format);

  if (status)
    {
      width=10;
      height=10;
    }

  gegl_operation_set_format (operation, "output", format);
  result.width  = width;
  result.height  = height;
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gint        problem;
  gpointer    format;
  gint        width, height;

  problem = query_png (o->path, &width, &height, &format);
  if (problem)
    {
      g_warning ("%s is %s really a PNG file?",
      G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }

  problem = gegl_buffer_import_png (output, o->path, 0, 0,
                                    &width, &height, format);

  if (problem)
    {
      g_warning ("%s failed to open file %s for reading.",
                 G_OBJECT_TYPE_NAME (operation), o->path);
      return FALSE;
    }

  return  TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  operation_class->name        = "gegl:png-load";
  operation_class->categories  = "hidden";
  operation_class->description = _("PNG image loader.");

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_extension_handler_register (".png", "gegl:png-load");
  gegl_extension_handler_register (".PNG", "gegl:png-load");
/*  done = TRUE; */
}

#endif
