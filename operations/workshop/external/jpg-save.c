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
gegl_chant_int    (quality, _("Quality"),
                   1, 100, 90, _("JPEG compression quality from 1 to 100"))
gegl_chant_int    (smoothing, _("Smoothing"),
                   0, 100, 0, _("Smoothing factor from 1 to 100; 0 disables smoothing"))
gegl_chant_boolean (progressive, _("Progressive"), TRUE,
                    _("Whether to create progressive JPEGs"))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "jpg-save.c"

#include "gegl-chant.h"
#include <glib/gstdio.h>
#include <jpeglib.h>
#include <jerror.h>
#include <stdio.h>

static gint
gegl_buffer_export_jpeg (GeglBuffer  *gegl_buffer,
                         const gchar *path,
                         gint         quality,
                         gint         smoothing,
                         gboolean     progressive,
                         gint         src_x,
                         gint         src_y,
                         gint         width,
                         gint         height)
{
  FILE                        *fp;
  struct jpeg_compress_struct  cinfo;
  struct jpeg_error_mgr        jerr;
  guchar                      *pixels;

  if (!strcmp (path, "-"))
    fp = stdout;
  else
    fp = fopen (path, "wb");

  if (!fp)
    return -1;

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  jpeg_stdio_dest (&cinfo, fp);

  cinfo.input_components = 3;
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);

  cinfo.smoothing_factor = smoothing;
  cinfo.optimize_coding = TRUE;

  if (progressive)
    jpeg_simple_progression (&cinfo);

  /* Use 1x1,1x1,1x1 MCUs and no subsampling */
  cinfo.comp_info[0].h_samp_factor = 1;
  cinfo.comp_info[0].v_samp_factor = 1;
  cinfo.comp_info[1].h_samp_factor = 1;
  cinfo.comp_info[1].v_samp_factor = 1;
  cinfo.comp_info[2].h_samp_factor = 1;
  cinfo.comp_info[2].v_samp_factor = 1;

  /* No restart markers */
  cinfo.restart_interval = 0;
  cinfo.restart_in_rows = 0;

  jpeg_start_compress (&cinfo, TRUE);

  pixels = g_malloc0 (cinfo.image_width * cinfo.input_components);

  while (cinfo.next_scanline < cinfo.image_height)
    {
      GeglRectangle rect;

      rect.x = src_x;
      rect.y = src_y + cinfo.next_scanline;
      rect.width = cinfo.image_width;
      rect.height = 1;

      gegl_buffer_get (gegl_buffer, 1.0, &rect,
                       babl_format ("R'G'B' u8"),
                       pixels, GEGL_AUTO_ROWSTRIDE);

      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &pixels, 1);
    }

  jpeg_finish_compress (&cinfo);
  jpeg_destroy_compress (&cinfo);

  g_free (pixels);

  if (stdout != fp)
    fclose (fp);

  return 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  gegl_buffer_export_jpeg (input, o->path, o->quality,
                           o->smoothing, o->progressive,
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

  sink_class->process    = process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:jpg-save";
  operation_class->categories  = "output";
  operation_class->description =
    _("JPEG image saver (passes the buffer through, saves as a side-effect.)");

  gegl_extension_handler_register_saver (".jpg", "gegl:jpg-save");
}

#endif
