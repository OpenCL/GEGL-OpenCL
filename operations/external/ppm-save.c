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
 * Copyright 2009 Henrik Akesson <h.m.akesson (a) gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string  (path, _("File"), "",
                    _("Target path and filename, use '-' for stdout."))
gegl_chant_boolean (rawformat, _("Raw format"), FALSE, _("Raw format"))

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "ppm-save.c"
        
#define CHANNEL_COUNT           3

#include "gegl-chant.h"
#include <stdio.h>

typedef enum {
  PIXMAP_ASCII  = 51,
  PIXMAP_RAW    = 54,
} map_type;

void
ppm_save_write(FILE     *fp,
               gint      width,
               gint      height,
               guchar   *pixels,
               map_type  type);

void
ppm_save_write(FILE    *fp,
               gint     width,
               gint     height,
               guchar  *pixels,
               map_type type)
{
  gint i, size, written;
  guchar * ptr;

  /* Write the header */
  fprintf (fp, "P%c\n%d %d\n", type, width, height );
  /* For the moment only 8 bit channels are supported */
  fprintf (fp, "%d\n", 255);

  size = width * height * sizeof (guchar) * CHANNEL_COUNT;

  /* Raw images writes the data in binary form */
  if (type == PIXMAP_RAW)
    {
      written = fwrite (pixels, 1, size, fp);
    }
  /* Otherwise a plain ascii format is used */
  else
    {
      ptr = pixels;

      for (i=0; i<size; i++)
        {
          fprintf (fp, "%3d ", (int) *ptr++);
          if ((i + 1) % (width * CHANNEL_COUNT) == 0)
            fprintf (fp, "\n");
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *rect)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  FILE     *fp;
  guchar   *pixels;
  map_type  type;

  fp = (!strcmp (o->path, "-") ? stdout : fopen(o->path, "wb") );
  if (!fp)
    {
      return FALSE;
    }

  pixels = g_malloc0 (rect->width * rect->height * 3);
  gegl_buffer_get (input, 1.0, rect, babl_format ("R'G'B' u8"), pixels,
          GEGL_AUTO_ROWSTRIDE);

  type = (o->rawformat ? PIXMAP_RAW : PIXMAP_ASCII);

  ppm_save_write (fp, rect->width, rect->height, pixels, type);

  g_free (pixels);
  if (fp != stdout)
    {
      fclose( fp );
    }

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:ppm-save";
  operation_class->categories  = "output";
  operation_class->description =
        _("PPM image saver (Portable pixmap saver.)");

}

#endif
