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


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
   description     (_("Path of file to load."))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE ppm-load.c

#define MAX_CHARS_IN_ROW        500
#define CHANNEL_COUNT           3
#define CHANNEL_COUNT_GRAY      1
#define ASCII_P                 'P'

#include "gegl-op.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef enum {
  PIXMAP_ASCII_GRAY = '2',
  PIXMAP_ASCII      = '3',
  PIXMAP_RAW_GRAY   = '5',
  PIXMAP_RAW        = '6',
} map_type;

typedef struct {
  map_type   type;
  glong      width;
  glong      height;
  gsize      numsamples; /* width * height * channels */
  gsize      channels;
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
    int    channel_count;

    /* Check the PPM file Type P3 or P6 */
    if (fgets (header, MAX_CHARS_IN_ROW, fp) == NULL ||
        header[0] != ASCII_P ||
        (header[1] != PIXMAP_ASCII_GRAY &&
         header[1] != PIXMAP_ASCII &&
         header[1] != PIXMAP_RAW_GRAY &&
         header[1] != PIXMAP_RAW))
      {
        g_warning ("Image is not a portable pixmap");
        return FALSE;
      }

    img->type = header[1];

    if (img->type == PIXMAP_RAW_GRAY || img->type == PIXMAP_ASCII_GRAY)
      channel_count = CHANNEL_COUNT_GRAY;
    else
      channel_count = CHANNEL_COUNT;

    /* Check the Comments */
    while((fgets (header, MAX_CHARS_IN_ROW, fp)) && (header[0] == '#'))
      ;

    /* Get Width and Height */
    errno = 0;
    img->width = strtol (header, &ptr, 10);
    if (errno)
      {
        g_warning ("Error reading width: %s", strerror(errno));
        return FALSE;
      }
    else if (img->width < 0)
      {
        g_warning ("Error: width is negative");
        return FALSE;
      }

    img->height = strtol (ptr, &ptr, 10);
    if (errno)
      {
        g_warning ("Error reading height: %s", strerror(errno));
        return FALSE;
      }
    else if (img->width < 0)
      {
        g_warning ("Error: height is negative");
        return FALSE;
      }

    if (fgets (header, MAX_CHARS_IN_ROW, fp))
      maxval = strtol (header, &ptr, 10);
    else
      maxval = 0;

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

    /* Later on, img->numsamples is multiplied with img->bpc to allocate
     * memory. Ensure it doesn't overflow. */
    if (!img->width || !img->height ||
        G_MAXSIZE / img->width / img->height / CHANNEL_COUNT < img->bpc)
      {
        g_warning ("Illegal width/height: %ld/%ld", img->width, img->height);
        return FALSE;
      }


    img->channels = channel_count;
    img->numsamples = img->width * img->height * channel_count;

    return TRUE;
}

static void
ppm_load_read_image(FILE       *fp,
                    pnm_struct *img)
{
    guint i;

    if (img->type == PIXMAP_RAW || img->type == PIXMAP_RAW_GRAY)
      {
        if (fread (img->data, img->bpc, img->numsamples, fp) == 0)
          return;

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
        /* Plain PPM or PGM format */

        if (img->bpc == sizeof (guchar))
          {
            guchar *ptr = img->data;

            for (i = 0; i < img->numsamples; i++)
              {
                guint sample;
                if (!fscanf (fp, " %u", &sample))
                  sample = 0;
                *ptr++ = sample;
              }
          }
        else if (img->bpc == sizeof (gushort))
          {
            gushort *ptr = (gushort *) img->data;

            for (i = 0; i < img->numsamples; i++)
              {
                guint sample;
                if (!fscanf (fp, " %u", &sample))
                  sample = 0;
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
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  pnm_struct    img;
  FILE         *fp;

  fp = (!strcmp (o->path, "-") ? stdin : fopen (o->path,"rb") );

  if (!fp)
    return result;

  if (!ppm_load_read_header (fp, &img))
    goto out;

  if (img.bpc == 1)
    {
      if (img.channels == 3)
        gegl_operation_set_format (operation, "output",
                                   babl_format ("R'G'B' u8"));
      else
        gegl_operation_set_format (operation, "output",
                                   babl_format ("Y' u8"));
    }
  else if (img.bpc == 2)
    {
      if (img.channels == 3)
        gegl_operation_set_format (operation, "output",
                                   babl_format ("R'G'B' u16"));
      else
        gegl_operation_set_format (operation, "output",
                                   babl_format ("Y' u16"));
    }
  else
    g_warning ("%s: Programmer stupidity error", G_STRLOC);

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
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  FILE         *fp;
  pnm_struct    img;
  GeglRectangle rect = {0,0,0,0};
  gboolean      ret = FALSE;

  fp = (!strcmp (o->path, "-") ? stdin : fopen (o->path,"rb"));

  if (!fp)
    return FALSE;

  if (!ppm_load_read_header (fp, &img))
    goto out;

  /* Allocating Array Size */

  /* Should use g_try_malloc(), but this causes crashes elsewhere because the
   * error signalled by returning FALSE isn't properly acted upon. Therefore
   * g_malloc() is used here which aborts if the requested memory size can't be
   * allocated causing a controlled crash. */
  img.data = (guchar*) g_malloc (img.numsamples * img.bpc);

  /* No-op without g_try_malloc(), see above. */
  if (! img.data)
    {
      g_warning ("Couldn't allocate %" G_GSIZE_FORMAT " bytes, giving up.", ((gsize)img.numsamples * img.bpc));
      goto out;
    }

  rect.height = img.height;
  rect.width = img.width;

  if (img.bpc == 1)
    {
      if (img.channels == 3)
        gegl_buffer_get (output, &rect, 1.0, babl_format ("R'G'B' u8"), img.data,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      else
        gegl_buffer_get (output, &rect, 1.0, babl_format ("Y' u8"), img.data,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  else if (img.bpc == 2)
    {
      if (img.channels == 3)
        gegl_buffer_get (output, &rect, 1.0, babl_format ("R'G'B' u16"), img.data,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      else
        gegl_buffer_get (output, &rect, 1.0, babl_format ("Y' u16"), img.data,
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  else
    g_warning ("%s: Programmer stupidity error", G_STRLOC);

  ppm_load_read_image (fp, &img);

  if (img.bpc == 1)
    {
      if (img.channels == 3)
        gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u8"), img.data,
                         GEGL_AUTO_ROWSTRIDE);
      else
        gegl_buffer_set (output, &rect, 0, babl_format ("Y' u8"), img.data,
                         GEGL_AUTO_ROWSTRIDE);
    }
  else if (img.bpc == 2)
    {
      if (img.channels == 3)
        gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u16"), img.data,
                         GEGL_AUTO_ROWSTRIDE);
      else
        gegl_buffer_set (output, &rect, 0, babl_format ("Y' u16"), img.data,
                         GEGL_AUTO_ROWSTRIDE);
    }
  else
    g_warning ("%s: Programmer stupidity error", G_STRLOC);

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
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:ppm-load",
    "title",        _("PPM File Loader"),
    "categories",   "hidden",
    "description",  _("PPM image loader."),
    NULL);

  gegl_extension_handler_register (".ppm", "gegl:ppm-load");
  gegl_extension_handler_register (".pgm", "gegl:ppm-load");
  gegl_extension_handler_register (".pnm", "gegl:ppm-load");
}

#endif
