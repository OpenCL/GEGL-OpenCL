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
 */
#if GEGL_CHANT_PROPERTIES
 
gegl_chant_string (path, "/tmp/fnord.png", "Target path and filename, use '-' for stdout.")
gegl_chant_int	  (compression, 1, 9, 1, "PNG compression level from 1 to 9")

#else

#define GEGL_CHANT_SINK
#define GEGL_CHANT_NAME        png_save
#define GEGL_CHANT_DESCRIPTION "PNG image saver (passes the buffer through, saves as a side-effect.)"
#define GEGL_CHANT_SELF        "png-save.c"
#define GEGL_CHANT_CATEGORIES      "output"
#include "gegl-chant.h"

#include <png.h>

gint
gegl_buffer_export_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
			gint	     compression,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height);

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation *self    = GEGL_CHANT_OPERATION (operation);
  GeglBuffer         *input;
  GeglRectangle      *result  = gegl_operation_result_rect (operation, context_id);

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  g_assert (input);

  gegl_buffer_export_png (input, self->path, self->compression,
                          result->x, result->y,
                          result->width, result->height);
  return  TRUE;
}

#include <stdio.h>

gint
gegl_buffer_export_png (GeglBuffer      *gegl_buffer,
                        const gchar     *path,
			gint		 compression,
                        gint             src_x,
                        gint             src_y,
                        gint             width,
                        gint             height)
{
  gint           row_stride = width * 4;
  FILE          *fp;
  gint           i;
  png_struct    *png;
  png_info      *info;
  guchar        *pixels;
  png_color_16   white;
  gchar          format_string[16];
  gint           bit_depth = 8;

  if (!strcmp (path, "-"))
    {
      fp = stdout;
    }
  else
    {
      fp = fopen (path, "wb");
    }
  if (!fp)
    {
      return -1;
    }

  strcpy (format_string, "R'G'B'A ");

  {
    Babl *babl= gegl_buffer->format;
    BablType   **type   = babl->format.type;

    for (i=0; i<babl->format.components; i++)
      if ((*type)->bits > 8)
        bit_depth = 16;
  }

  if (bit_depth == 16)
    {
      strcat (format_string, "u16");
      row_stride *= 2;
    }
  else
    {
      strcat (format_string, "u8");
    }

  png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
    return -1;

  info = png_create_info_struct (png);

  if (setjmp (png_jmpbuf (png)))
    return -1;
  png_set_compression_level (png, compression);
  png_init_io (png, fp);

  png_set_IHDR (png, info,
     width, height, bit_depth, PNG_COLOR_TYPE_RGB_ALPHA,
     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

  white.red = 0xff;
  white.blue = 0xff;
  white.green = 0xff;
  png_set_bKGD (png, info, &white);

  png_write_info (png, info);

#if BYTE_ORDER == LITTLE_ENDIAN
  if (bit_depth > 8)
    png_set_swap (png);
#endif

  pixels = g_malloc0 (row_stride);

  for (i=0; i< height; i++)
    {
      GeglRectangle rect = {src_x, src_y+i, width, 1};
      gegl_buffer_get (gegl_buffer, 1.0, &rect, babl_format (format_string), pixels, GEGL_AUTO_ROWSTRIDE);

      png_write_rows (png, &pixels, 1);
    }

  png_write_end (png, info);

  png_destroy_write_struct (&png, &info);
  g_free (pixels);

  if (fp!=stdout)
    fclose (fp);

  return 0;
}

#endif
