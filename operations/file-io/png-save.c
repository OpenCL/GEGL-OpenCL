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
 
gegl_chant_string(path, "/tmp/fnord.png", "path to new file to save - for stdout")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME        png_save
#define GEGL_CHANT_DESCRIPTION "saves a png image using libpng"
#define GEGL_CHANT_SELF        "png-save.c"
#define GEGL_CHANT_CATEGORIES      "output"
#include "gegl-chant.h"

#include <png.h>

gint
gegl_buffer_export_png (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height);

static gboolean
evaluate (GeglOperation *operation,
          const gchar *output_prop)
{
  ChantInstance *self = GEGL_CHANT_INSTANCE (operation);
  GeglOperationFilter *op_filter = GEGL_OPERATION_FILTER (operation);
  GeglBuffer          *input   = op_filter->input;
  GeglRect            *result  = gegl_operation_result_rect (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  /*g_warning ("%s", ((Babl*)(input->format))->instance.name);*/

  g_assert (input);

  gegl_buffer_export_png (input, self->path,
                          result->x, result->y,
                          result->w, result->h);

  op_filter->output = NULL;
  return  TRUE;
}

#include <stdio.h>

gint
gegl_buffer_export_png (GeglBuffer      *gegl_buffer,
                        const gchar     *path,
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
  
  png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
    return -1;

  info = png_create_info_struct (png);

  if (setjmp (png_jmpbuf (png)))
    return -1;
  png_set_compression_level (png, 1);
  png_init_io (png, fp);

  png_set_IHDR (png, info,
     width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

  white.red = 0xff;
  white.blue = 0xff;
  white.green = 0xff;
  png_set_bKGD (png, info, &white);

  png_write_info (png, info);
  pixels = g_malloc0 (row_stride);

  for (i=0; i< height; i++)
    {
      GeglBuffer *rect = g_object_new (GEGL_TYPE_BUFFER,
                                       "source", gegl_buffer,
                                       "x", src_x,
                                       "y", src_y + i,
                                       "width", width,
                                       "height", 1,
                                       NULL);
      gegl_buffer_get_fmt (rect, pixels, babl_format ("R'G'B'A u8"));
      png_write_rows (png, &pixels, 1);
      g_object_unref (rect);
    }

  png_write_end (png, info);

  png_destroy_write_struct (&png, &info);
  g_free (pixels);

  if (fp!=stdout)
    fclose (fp);

  return 0;
}

#endif
