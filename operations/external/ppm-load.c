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
        gsize      numsamples; /* width * height * channels */
        gsize      bpc;        /* bytes per channel */
	guchar    *data;
} pnm_struct;

static gboolean
ppm_load_read_header(FILE       *fp,
                     pnm_struct *img)
{
    /* PPM Headers Variable Declaration */
    gchar *ptr;
    //gchar *retval;
    gchar  header[MAX_CHARS_IN_ROW];
    gint   maxval;

    /* Check the PPM file Type P2 or P5 */
    fgets (header,MAX_CHARS_IN_ROW,fp);

    if (header[0] != ASCII_P ||
        (header[1] != PIXMAP_ASCII &&
         header[1] != PIXMAP_RAW))
      {
        g_warning ("Image is not a portable pixmap");
        return FALSE;
      }

    img->type = header[1];

    /* Check the Comments */
    fgets (header,MAX_CHARS_IN_ROW,fp);
    while(header[0] == '#')
      {
        fgets (header,MAX_CHARS_IN_ROW,fp);
      }

    /* Get Width and Height */
    img->width  = strtol (header,&ptr,0);
    img->height = atoi (ptr);
    img->numsamples = img->width * img->height * CHANNEL_COUNT;

    fgets (header,MAX_CHARS_IN_ROW,fp);
    maxval = strtol (header,&ptr,0);

    if ((maxval != 255) && (maxval != 65535))
      {
        g_warning ("Image is not an 8-bit or 16-bit portable pixmap");
        return FALSE;
      }

  switch (maxval)
    {
    case 255:
      img->bpc = sizeof (guchar);
      break;

    case 65535:
      img->bpc = sizeof (gushort);
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
    guint   i;

    if (img->type == PIXMAP_RAW)
      {
        fread (img->data, img->bpc, img->numsamples, fp);

        /* Fix endianness if necessary */
        if (img->bpc > 1)
          {
            gushort *ptr = (gushort *) img->data;

            for (i=0; i < img->numsamples; i++)
              {
                *ptr = GUINT16_FROM_BE (*ptr);
                ptr++;
              }
          }
      }
    else
      {
        /* Plain PPM format */

        if (img->bpc == sizeof (guchar))
          {
            guchar *ptr = img->data;

            for (i = 0; i < img->numsamples; i++)
              {
                guint sample;
                fscanf (fp, " %u", &sample);
                *ptr++ = sample;
              }
          }
        else if (img->bpc == sizeof (gushort))
          {
            gushort *ptr = (gushort *) img->data;

            for (i = 0; i < img->numsamples; i++)
              {
                guint sample;
                fscanf (fp, " %u", &sample);
                *ptr++ = sample;
              }
          }
        else
          {
            g_warning ("%s: Programmer stupidity error", G_STRLOC);
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
    return result;

  if (!ppm_load_read_header (fp, &img))
    goto out;

  switch (img.bpc)
    {
    case 1:
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B' u8"));
      break;

    case 2:
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B' u16"));
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  result.width = img.width;
  result.height = img.height;

 out:
  if (stdin != fp)
    fclose (fp);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  FILE         *fp;
  pnm_struct    img;
  GeglRectangle rect = {0,0,0,0};
  gboolean      ret = FALSE;

  fp = (!strcmp (o->path, "-") ? stdin : fopen (o->path,"rb"));

  if (!fp)
    return FALSE;

  if (!ppm_load_read_header (fp, &img))
    goto out;

  rect.height = img.height;
  rect.width = img.width;

  /* Allocating Array Size */
  img.data = (guchar*) g_malloc (img.numsamples * img.bpc);

  switch (img.bpc)
    {
    case 1:
      gegl_buffer_get (output, &rect, 1.0, babl_format ("R'G'B' u8"), img.data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      break;

    case 2:
      gegl_buffer_get (output, &rect, 1.0, babl_format ("R'G'B' u16"), img.data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  ppm_load_read_image (fp, &img);

  switch (img.bpc)
    {
    case 1:
      gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u8"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    case 2:
      gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u16"), img.data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  g_free (img.data);

  ret = TRUE;

 out:
  if (stdin != fp)
    fclose (fp);

  return ret;
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

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:ppm-load",
    "categories"  , "hidden",
    "description" , _("PPM image loader."),
    NULL);

  gegl_extension_handler_register (".ppm", "gegl:ppm-load");
}

#endif
