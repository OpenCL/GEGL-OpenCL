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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, _("File"), "",
                   _("Target path and filename, use '-' for stdout."))
gegl_chant_int    (compression, _("Compression"),
                   1, 9, 1, _("PNG compression level from 1 to 9"))
gegl_chant_int    (bitdepth, _("Bitdepth"),
                   8, 16, 16, _("8 and 16 are amongst the currently accepted values."))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "png-save.c"

#include "gegl-chant.h"
#include <png.h>
#include <stdio.h>

/* this call is available when the png-save plug-in is loaded,
 * it might have to be dlsymed to be used?
 */
gint
gegl_buffer_export_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         compression,
                        gint         bd,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height);

gint
gegl_buffer_export_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         compression,
                        gint         bd,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height)
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
    const Babl *babl; /*= gegl_buffer->format;*/

    g_object_get (gegl_buffer, "format", &babl, NULL);

    if (babl_format_get_type (babl, 0) != babl_type ("u8"))
      bit_depth = 16;

    if (bd == 16)
      bit_depth = 16;
    else
      bit_depth = 8;
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
    {
      if (stdout != fp)
        fclose (fp);

      return -1;
    }

  info = png_create_info_struct (png);

  if (setjmp (png_jmpbuf (png)))
    {
      if (stdout != fp)
        fclose (fp);

      return -1;
    }

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
      GeglRectangle rect;

      rect.x = src_x;
      rect.y = src_y+i;
      rect.width = width;
      rect.height = 1;

      gegl_buffer_get (gegl_buffer, 1.0, &rect, babl_format (format_string), pixels, GEGL_AUTO_ROWSTRIDE);

      png_write_rows (png, &pixels, 1);
    }

  png_write_end (png, info);

  png_destroy_write_struct (&png, &info);
  g_free (pixels);

  if (stdout != fp)
    fclose (fp);

  return 0;
}

static gboolean
gegl_png_save_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_buffer_export_png (input, o->path, o->compression, o->bitdepth,
                          result->x, result->y,
                          result->width, result->height);
  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process    = gegl_png_save_process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:png-save";
  operation_class->categories  = "output";
  operation_class->description =
        _("PNG image saver (passes the buffer through, saves as a side-effect.)");

}

#endif
