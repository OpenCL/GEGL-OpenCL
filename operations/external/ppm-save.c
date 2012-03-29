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
gegl_chant_boolean (rawformat, _("Raw format"), TRUE, _("Raw format"))
gegl_chant_int     (bitdepth, _("Bitdepth"),
                    8, 16, 16,
                    _("8 and 16 are amongst the currently accepted values."))

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

static void
ppm_save_write(FILE    *fp,
               gint     width,
               gint     height,
               gsize    numsamples,
               gsize    bpc,
               guchar  *data,
               map_type type)
{
  guint i;

  /* Write the header */
  fprintf (fp, "P%c\n%d %d\n", type, width, height );
  fprintf (fp, "%d\n", (bpc == sizeof (guchar)) ? 255 : 65535);

  /* Raw images writes the data in binary form */
  if (type == PIXMAP_RAW)
    {
      /* Fix endianness if necessary */
      if (bpc > 1)
        {
          gushort *ptr = (gushort *) data;

          for (i = 0; i < numsamples; i++)
            {
              *ptr = GUINT16_TO_BE (*ptr);
              ptr++;
            }
        }

      fwrite (data, bpc, numsamples, fp);
    }
  else
    {
      /* Plain PPM format */

      if (bpc == sizeof (guchar))
        {
          guchar *ptr = data;

          for (i = 0; i < numsamples; i++)
            {
              fprintf (fp, "%u ", (unsigned int) *ptr++);
              if ((i + 1) % (width * CHANNEL_COUNT) == 0)
                fprintf (fp, "\n");
            }
        }
      else if (bpc == sizeof (gushort))
        {
          gushort *ptr = (gushort *) data;

          for (i = 0; i < numsamples; i++)
            {
              fprintf (fp, "%u ", (unsigned int) *ptr++);
              if ((i + 1) % (width * CHANNEL_COUNT) == 0)
                fprintf (fp, "\n");
            }
        }
      else
        {
          g_warning ("%s: Programmer stupidity error", G_STRLOC);
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *rect,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  FILE     *fp;
  guchar   *data;
  map_type  type;
  gsize     bpc;
  gsize     numsamples;
  gboolean  ret = FALSE;

  fp = (!strcmp (o->path, "-") ? stdout : fopen(o->path, "wb") );

  if (!fp)
    return FALSE;

  if ((o->bitdepth != 8) && (o->bitdepth != 16))
    {
      g_warning ("Bitdepths of 8 and 16 are only accepted currently.");
      goto out;
    }

  type = (o->rawformat ? PIXMAP_RAW : PIXMAP_ASCII);
  bpc = (o->bitdepth == 8) ? (sizeof (guchar)) : (sizeof (gushort));
  numsamples = rect->width * rect->height * CHANNEL_COUNT;

  data = g_malloc (numsamples * bpc);

  switch (bpc)
    {
    case 1:
      gegl_buffer_get (input, rect, 1.0, babl_format ("R'G'B' u8"), data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      break;

    case 2:
      gegl_buffer_get (input, rect, 1.0, babl_format ("R'G'B' u16"), data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  ppm_save_write (fp, rect->width, rect->height, numsamples, bpc, data, type);

  g_free (data);

  ret = TRUE;

 out:
  if (fp != stdout)
    fclose( fp );

  return ret;
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

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:ppm-save",
    "categories"  , "output",
    "description" ,
        _("PPM image saver (Portable pixmap saver.)"),
        NULL);

  gegl_extension_handler_register_saver (".ppm", "gegl:ppm-save");
}

#endif
