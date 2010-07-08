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
 * Copyright (c) 2010 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "jp2-load.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <jasper/jasper.h>

static gboolean
query_jp2 (const gchar   *path,
           gint          *width,
           gint          *height,
           gint          *depth,
           jas_image_t  **jas_image)
{
  gboolean ret;
  jas_stream_t *in;
  int image_fmt;
  jas_image_t *image;
  jas_cmprof_t *output_profile;
  jas_image_t *cimage;
  int numcmpts;
  int i;
  gboolean b;

  in = NULL;
  cimage = image = NULL;
  output_profile = NULL;
  ret = FALSE;

  do
    {
      in = jas_stream_fopen (path, "rb");
      if (!in)
	{
	  g_warning ("Unable to open image file '%s'", path);
	  break;
	}

      image_fmt = jas_image_getfmt (in);
      if (image_fmt < 0)
	{
	  g_warning (_("Unknown JPEG-2000 image format in '%s'"), path);
          break;
	}

      image = jas_image_decode (in, image_fmt, NULL);
      if (!image)
	{
	  g_warning (_("Unable to open JPEG-2000 image in '%s'"), path);
	  break;
	}

      output_profile = jas_cmprof_createfromclrspc (JAS_CLRSPC_SRGB);
      if (!output_profile)
        {
	  g_warning (_("Unable to create output color profile for '%s'"), path);
	  break;
        }

      cimage = jas_image_chclrspc (image, output_profile,
                                   JAS_CMXFORM_INTENT_PER);
      if (!cimage)
        {
	  g_warning (_("Unable to convert image to sRGB color space "
                       "when processing '%s'"), path);
	  break;
        }

      numcmpts = jas_image_numcmpts (cimage);
      if (numcmpts != 3)
	{
	  g_warning (_("Unsupported non-RGB JPEG-2000 file with "
                       "%d components in '%s'"), numcmpts, path);
	  break;
	}

      *width = jas_image_cmptwidth (cimage, 0);
      *height = jas_image_cmptheight (cimage, 0);
      *depth = jas_image_cmptprec (cimage, 0);

      if ((*depth != 8) && (*depth != 16))
	{
	  g_warning (_("Unsupported JPEG-2000 file with depth %d in '%s'"),
                     *depth, path);
	  break;
	}

      b = FALSE;

      for (i = 1; i < 3; i++)
        {
          if ((jas_image_cmptprec (cimage, i) != *depth) ||
              (jas_image_cmptwidth (cimage, i) != *width) ||
              (jas_image_cmptheight (cimage, i) != *height))
            {
              g_warning (_("Components of input image '%s' don't match"),
                         path);
              b = TRUE;
              break;
            }
        }

      if (b)
        break;

      ret = TRUE;
    }
  while (FALSE); /* structured goto */

  if (jas_image)
    *jas_image = cimage;
  else if (cimage)
    jas_image_destroy (cimage);

  if (image)
    jas_image_destroy (image);

  if (output_profile)
    jas_cmprof_destroy (output_profile);

  if (in)
    jas_stream_close (in);

  return ret;
}

static void
prepare (GeglOperation *operation)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      jas_init ();
      initialized = TRUE;
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle rect = {0,0,0,0};
  jas_image_t *image;
  gint width, height, depth;
  gsize bpc;
  guchar *data = NULL;
  gboolean ret;
  int components[3];
  jas_matrix_t *matrices[3] = {NULL, NULL, NULL};
  gint i;
  gint row;
  gboolean b;
  gushort *ptr_s;
  guchar *ptr_b;

  image = NULL;
  width = height = depth = 0;

  if (!query_jp2 (o->path, &width, &height, &depth, &image))
    return FALSE;

  rect.height = height;
  rect.width = width;

  switch (depth)
    {
    case 8:
      bpc = sizeof (guchar);
      break;

    case 16:
      bpc = sizeof (gushort);
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
      return FALSE;
    }

  data = (guchar *) g_malloc (width * height * 3 * bpc);
  ptr_s = (gushort *) data;
  ptr_b = data;

  switch (depth)
    {
    case 16:
      gegl_buffer_get (output, 1.0, &rect, babl_format ("R'G'B' u16"), data,
                       GEGL_AUTO_ROWSTRIDE);
      break;

    case 8:
    default:
      gegl_buffer_get (output, 1.0, &rect, babl_format ("R'G'B' u8"), data,
                       GEGL_AUTO_ROWSTRIDE);
    }

  ret = FALSE;
  b = FALSE;

  do
    {
      components[0] = jas_image_getcmptbytype
        (image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_R));
      components[1] = jas_image_getcmptbytype
        (image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_G));
      components[2] = jas_image_getcmptbytype
        (image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_B));

      if ((components[0] < 0) || (components[1] < 0) || (components[2] < 0))
        {
          g_warning (_("One or more of R, G, B components are missing "
                       "from '%s'"), o->path);
          break;
        }

      if (jas_image_cmptsgnd (image, components[0]) ||
          jas_image_cmptsgnd (image, components[1]) ||
          jas_image_cmptsgnd (image, components[2]))
        {
          g_warning (_("One or more of R, G, B components have signed "
                       "data in '%s'"), o->path);
          break;
        }

      for (i = 0; i < 3; i++)
        matrices[i] = jas_matrix_create(1, width);

      for (row = 0; row < height; row++)
        {
          gint plane, col;
          jas_seqent_t *jrow[3] = {NULL, NULL, NULL};

          for (plane = 0; plane < 3; plane++)
            {
              int r = jas_image_readcmpt (image, components[plane], 0, row,
                                          width, 1, matrices[plane]);
              if (r)
                {
                  g_warning (_("Error reading row %d component %d from '%s'"),
                             row, plane, o->path);
                  b = TRUE;
                  break;
                }
            }

          if (b)
            break;

          for (plane = 0; plane < 3; plane++)
            jrow[plane] = jas_matrix_getref (matrices[plane], 0, 0);

          for (col = 0; col < width; col++)
            {
              switch (depth)
                {
                case 16:
                  *ptr_s++ = (gushort) jrow[0][col];
                  *ptr_s++ = (gushort) jrow[1][col];
                  *ptr_s++ = (gushort) jrow[2][col];
                  break;

                case 8:
                default:
                  *ptr_b++ = (guchar) jrow[0][col];
                  *ptr_b++ = (guchar) jrow[1][col];
                  *ptr_b++ = (guchar) jrow[2][col];
                }
            }
        }

      if (b)
        break;

      switch (depth)
        {
        case 16:
          gegl_buffer_set (output, &rect, babl_format ("R'G'B' u16"), data,
                           GEGL_AUTO_ROWSTRIDE);
          break;

        case 8:
        default:
          gegl_buffer_set (output, &rect, babl_format ("R'G'B' u8"), data,
                           GEGL_AUTO_ROWSTRIDE);
        }

      ret = TRUE;
    }
  while (FALSE); /* structured goto */

  for (i = 0; i < 3; i++)
    if (matrices[i])
      jas_matrix_destroy (matrices[i]);

  if (data)
    g_free (data);

  if (image)
    jas_image_destroy (image);

  return ret;
}

static GeglRectangle
get_bounding_box (GeglOperation * operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  gint width, height, depth;

  width = height = depth = 0;

  if (!query_jp2 (o->path, &width, &height, &depth, NULL))
    return result;

  result.width = width;
  result.height = height;

  switch (depth)
    {
    case 16:
      gegl_operation_set_format (operation, "output",
				 babl_format ("R'G'B' u16"));
      break;

    case 8:
      gegl_operation_set_format (operation, "output",
				 babl_format ("R'G'B' u8"));
      break;

    default:
      g_warning ("%s: Programmer stupidity error", G_STRLOC);
    }

  return result;
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
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  operation_class->name        = "gegl:jp2-load";
  operation_class->categories  = "hidden";
  operation_class->description = _("JPEG-2000 image loader.");

  gegl_extension_handler_register (".jp2", "gegl:jp2-load");
  gegl_extension_handler_register (".jpx", "gegl:jp2-load");
}

#endif
