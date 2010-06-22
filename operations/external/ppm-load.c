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

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "ppm-load.c"

#define MAX_CHARS_IN_ROW        500
#define CHANNEL_COUNT           3
#define ASCII_P                 80

#include "gegl-chant.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
  PIXMAP_ASCII  = 51,
  PIXMAP_RAW    = 54,
} map_type;

typedef struct {
	map_type   type;
	gint       width;
	gint       height;
        gsize      size;
	gint       maxval;
	guchar    *data;
} pnm_struct;

static gboolean
ppm_load_read_header(FILE       *fp,
                     pnm_struct *img)
{
    /* PPM Headers Variable Declaration */
    gchar *ptr;
    gchar *retval;
    gchar  header[MAX_CHARS_IN_ROW];

    /* Check the PPM file Type P2 or P5 */
    retval = fgets (header,MAX_CHARS_IN_ROW,fp);

    if (header[0] != ASCII_P ||
        (header[1] != PIXMAP_ASCII &&
         header[1] != PIXMAP_RAW))
      {
        g_warning ("Image is not a portable pixmap");
        return FALSE;
      }

    img->type = header[1];

    /* Check the Comments */
    retval = fgets (header,MAX_CHARS_IN_ROW,fp);
    while(header[0] == '#')
      {
        retval = fgets (header,MAX_CHARS_IN_ROW,fp);
      }

    /* Get Width and Height */
    img->width  = strtol (header,&ptr,0);
    img->height = atoi (ptr);

    retval = fgets (header,MAX_CHARS_IN_ROW,fp);
    img->maxval = strtol (header,&ptr,0);

    if ((img->maxval != 255) && (img->maxval != 65535))
      {
        g_warning ("Image is not an 8-bit or 16-bit portable pixmap");
        return FALSE;
      }

  switch (img->maxval)
    {
    case 255:
      img->size = img->width * img->height * sizeof (guchar) * CHANNEL_COUNT;
      break;

    case 65535:
      img->size = img->width * img->height * sizeof (gushort) * CHANNEL_COUNT;
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

    return TRUE;
}

static void
ppm_load_read_image(FILE       *fp,
                    pnm_struct *img)
{
    gint    i;
    gint    retval;

    if (img->type == PIXMAP_RAW)
      {
        retval = fread (img->data, 1, img->size, fp);

        /* Fix endianness if necessary */
        if (img->maxval > 255)
          {
            gushort *ptr = (gushort *) img->data;

            for (i=0; i < (img->width * img->height * CHANNEL_COUNT); i++)
              {
                *ptr = GUINT16_FROM_BE (*ptr);
                ptr++;
              }
          }
      }
    else
      {
        /* Plain PPM format */

        if (img->maxval < 256)
          {
            guchar *ptr = img->data;

            for (i=0; i<img->size; i++)
              {
                int sample;
                retval = fscanf (fp, " %d", &sample);
                *ptr++ = sample;
              }
          }
        else
          {
            gushort *ptr = (gushort *) img->data;

            for (i=0; i < (img->width * img->height * CHANNEL_COUNT); i++)
              {
                int sample;
                retval = fscanf (fp, " %d", &sample);
                *ptr++ = sample;
              }
          }
      }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  pnm_struct    img;
  FILE         *fp;

  fp = (!strcmp (o->path, "-") ? stdin : fopen (o->path,"rb") );

  if (!fp)
    {
      return result;
    }

  if (!ppm_load_read_header (fp, &img))
    {
      return result;
    }

  if (stdin != fp)
    {
      fclose (fp);
    }

  switch (img.maxval)
    {
    case 255:
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B' u8"));
      break;

    case 65535:
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B' u16"));
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  result.width = img.width;
  result.height = img.height;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  FILE         *fp;
  pnm_struct    img;
  GeglRectangle rect = {0,0,0,0};

  fp = (!strcmp (o->path, "-") ? stdin : fopen (o->path,"rb"));

  if (!fp)
    {
      return FALSE;
    }

  if (!ppm_load_read_header (fp, &img))
    {
      return FALSE;
    }

  rect.height = img.height;
  rect.width = img.width;

  /* Allocating Array Size */
  img.data = (guchar*) g_malloc0 (img.size);

  switch (img.maxval)
    {
    case 255:
      gegl_buffer_get (output, 1.0, &rect, babl_format ("R'G'B' u8"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    case 65535:
      gegl_buffer_get (output, 1.0, &rect, babl_format ("R'G'B' u16"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  ppm_load_read_image (fp, &img);

  switch (img.maxval)
    {
    case 255:
      gegl_buffer_set (output, &rect, babl_format ("R'G'B' u8"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    case 65535:
      gegl_buffer_set (output, &rect, babl_format ("R'G'B' u16"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  g_free (img.data);
  if (stdin != fp)
    {
      fclose (fp);
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

  operation_class->name        = "gegl:ppm-load";
  operation_class->categories  = "hidden";
  operation_class->description = _("PPM image loader.");

  gegl_extension_handler_register (".ppm", "gegl:ppm-load");
}

#endif
