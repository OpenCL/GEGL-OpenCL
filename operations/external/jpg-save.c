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
 * Copyright 2011 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, _("File"), "",
                   _("Target path and filename, use '-' for stdout"))
gegl_chant_int    (quality, _("Quality"), 1, 100, 90,
                   _("JPEG compression quality (between 1 and 100)"))
gegl_chant_int    (smoothing, _("Smoothing"),
                   0, 100, 0, _("Smoothing factor from 1 to 100; 0 disables smoothing"))
gegl_chant_boolean (optimize, _("Optimize"), TRUE,
                    _("Use optimized huffman tables"))
gegl_chant_boolean (progressive, _("Progressive"), TRUE,
                    _("Create progressive JPEG images"))
gegl_chant_boolean (grayscale, _("Grayscale"), FALSE,
                    _("Create a grayscale (monochrome) image"))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "jpg-save.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <jpeglib.h>

static gint
gegl_buffer_export_jpg (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         quality,
                        gint         smoothing,
                        gboolean     optimize,
                        gboolean     progressive,
                        gboolean     grayscale,
                        gint         src_x,
                        gint         src_y,
                        gint         width,
                        gint         height)
{
  FILE *fp;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  const Babl *format;

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

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  jpeg_stdio_dest (&cinfo, fp);

  cinfo.image_width = width;
  cinfo.image_height = height;

  if (!grayscale)
    {
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
    }
  else
    {
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    }

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
  cinfo.smoothing_factor = smoothing;
  cinfo.optimize_coding = optimize;
  if (progressive)
    jpeg_simple_progression (&cinfo);

  /* Use 1x1,1x1,1x1 MCUs and no subsampling */
  cinfo.comp_info[0].h_samp_factor = 1;
  cinfo.comp_info[0].v_samp_factor = 1;

  if (!grayscale)
    {
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
    }

  /* No restart markers */
  cinfo.restart_interval = 0;
  cinfo.restart_in_rows = 0;

  jpeg_start_compress (&cinfo, TRUE);

  if (!grayscale)
    {
      format = babl_format ("R'G'B' u8");
      row_pointer[0] = g_malloc (width * 3);
    }
  else
    {
      format = babl_format ("Y' u8");
      row_pointer[0] = g_malloc (width);
    }

  while (cinfo.next_scanline < cinfo.image_height) {
    GeglRectangle rect;

    rect.x = src_x;
    rect.y = src_y + cinfo.next_scanline;
    rect.width = width;
    rect.height = 1;

    gegl_buffer_get (gegl_buffer, &rect, 1.0, format,
                     row_pointer[0], GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);

    jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);
  jpeg_destroy_compress (&cinfo);

  g_free (row_pointer[0]);

  if (stdout != fp)
    fclose (fp);

  return 0;
}

static gboolean
gegl_jpg_save_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       const GeglRectangle *result,
                       int                  level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_buffer_export_jpg (input, o->path, o->quality, o->smoothing,
                          o->optimize, o->progressive, o->grayscale,
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

  sink_class->process    = gegl_jpg_save_process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:jpg-save",
    "categories"  , "output",
    "description" ,
    _("JPEG image saver (passes the buffer through, saves as a side-effect)"),
    NULL);

  gegl_extension_handler_register_saver (".jpg", "gegl:jpg-save");
}

#endif
