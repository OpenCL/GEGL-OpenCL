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
 * Copyright (c) 2010, 2011 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI for file to load"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME jp2_load
#define GEGL_OP_C_SOURCE jp2-load.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <jasper/jasper.h>

typedef struct
{
  GFile *file;

  jas_image_t *image;

  const Babl *format;

  gint width;
  gint height;
} Priv;

static void
cleanup(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      if (p->image != NULL)
        jas_image_destroy(p->image);
      p->image = NULL;

      if (p->file != NULL)
        g_clear_object(&p->file);

      p->width = p->height = 0;
      p->format = NULL;
    }
}

static gssize
read_from_stream(GInputStream *stream,
                 jas_stream_t *jasper)
{
  GError *error = NULL;
  gchar *buffer;
  const gsize size = 4096;
  gssize read = -1;

  buffer = g_try_new(gchar, size);

  g_assert(buffer != NULL);

  do
    {
      read = g_input_stream_read(G_INPUT_STREAM(stream),
                                 (void *) buffer, size,
                                 NULL, &error);
      if (read < 0)
        {
          g_warning("%s", error->message);
          g_error_free(error);
          break;
        }
      else if (read > 0)
        jas_stream_write(jasper, buffer, read);
      else
        jas_stream_rewind(jasper);
    }
  while (read != 0);

  return read;
}

static gboolean
query_jp2(GeglOperation *operation,
          jas_stream_t *jasper)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  gboolean ret;
  int image_fmt;
  jas_image_t *image;
  jas_cmprof_t *output_profile;
  gint depth;
  int numcmpts;
  int i;
  gboolean b;

  g_return_val_if_fail(jasper != NULL, FALSE);

  image = NULL;
  output_profile = NULL;
  ret = FALSE;

  do
    {
      image_fmt = jas_image_getfmt (jasper);
      if (image_fmt < 0)
        {
          g_warning ("%s", _("Unknown JPEG 2000 image format"));
          break;
        }

      image = jas_image_decode (jasper, image_fmt, NULL);
      if (!image)
        {
          g_warning ("%s", _("Unable to open JPEG 2000 image"));
          break;
        }

      output_profile = jas_cmprof_createfromclrspc (JAS_CLRSPC_SRGB);
      if (!output_profile)
        {
          g_warning ("%s", _("Unable to create output color profile"));
          break;
        }

      p->image = jas_image_chclrspc (image, output_profile,
                                     JAS_CMXFORM_INTENT_PER);
      if (!p->image)
        {
          g_warning ("%s", _("Unable to convert image to sRGB color space"));
          break;
        }

      numcmpts = jas_image_numcmpts (p->image);
      if (numcmpts != 3)
        {
          g_warning (_("Unsupported non-RGB JPEG 2000 file with "
                       "%d components"), numcmpts);
          break;
        }

      p->width = jas_image_cmptwidth (p->image, 0);
      p->height = jas_image_cmptheight (p->image, 0);

      depth = jas_image_cmptprec (p->image, 0);

      if ((depth != 8) && (depth != 16))
        {
          g_warning (_("Unsupported JPEG 2000 file with depth %d"), depth);
          break;
        }

      switch (depth)
        {
        case 16:
          p->format = babl_format("R'G'B' u16");
          break;

        case 8:
          p->format = babl_format("R'G'B' u8");
          break;

        default:
          g_warning ("%s: Programmer stupidity error", G_STRLOC);
        }

      b = FALSE;

      for (i = 1; i < 3; i++)
        {
          if ((jas_image_cmptprec (p->image, i) != depth) ||
              (jas_image_cmptwidth (p->image, i) != p->width) ||
              (jas_image_cmptheight (p->image, i) != p->height))
            {
              g_warning ("%s", _("Components of JPEG 2000 input don't match"));
              b = TRUE;
              break;
            }
        }

      if (b)
        break;

      ret = TRUE;
    }
  while (FALSE); /* structured goto */

  if (image)
    jas_image_destroy (image);

  if (output_profile)
    jas_cmprof_destroy (output_profile);

  return ret;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (o->user_data) ? o->user_data : g_new0(Priv, 1);
  static gboolean initialized = FALSE;
  jas_stream_t *jasper;
  GInputStream *stream;
  GError *error = NULL;
  GFile *file = NULL;

  g_assert(p != NULL);

  if (!initialized)
    {
      jas_init ();
      initialized = TRUE;
    }

  if (p->file != NULL && (o->uri || o->path))
    {
      if (o->uri && strlen(o->uri) > 0)
        file = g_file_new_for_uri(o->uri);
      else if (o->path && strlen(o->path) > 0)
        file = g_file_new_for_path(o->path);
      if (file != NULL)
        {
          if (!g_file_equal(p->file, file))
            cleanup(operation);
          g_object_unref(file);
        }
    }

  o->user_data = (void*) p;

  if (p->image == NULL)
    {
      jasper = jas_stream_memopen(NULL, -1);
      if (jasper == NULL)
        {
          g_warning("%s", _("could not create a new Jasper stream"));
          cleanup(operation);
          return;
        }

      stream = gegl_gio_open_input_stream(o->uri, o->path, &p->file, &error);
      if (stream == NULL)
        {
          g_warning("%s", error->message);
          g_error_free(error);
          cleanup(operation);
          return;
        }

      if (read_from_stream(stream, jasper) < 0)
        {
          if (o->uri != NULL && strlen(o->uri) > 0)
            g_warning(_("failed to open JPEG 2000 from %s"), o->uri);
          else
            g_warning(_("failed to open JPEG 2000 from %s"), o->path);
          g_input_stream_close(G_INPUT_STREAM(stream), NULL, NULL);
          jas_stream_close(jasper);

          g_object_unref(stream);
          cleanup(operation);
          return;
        }

      g_input_stream_close(G_INPUT_STREAM(stream), NULL, NULL);
      g_object_unref(stream);

      if (!query_jp2(operation, jasper))
        {
          g_warning("%s", _("could not query JPEG 2000 file"));
          cleanup(operation);
          return;
        }

      jas_stream_close(jasper);
    }

  gegl_operation_set_format(operation, "output", p->format);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  GeglRectangle rect = {0,0,0,0};
  const Babl *type;
  gint depth;
  guchar *data_b;
  gushort *data_s;
  gboolean ret;
  int components[3];
  jas_matrix_t *matrices[3] = {NULL, NULL, NULL};
  gint i;
  gint row;
  gboolean b;

  data_b = NULL;
  data_s = NULL;

  depth = 0;

  type = babl_format_get_type(p->format, 0);
  if (type == babl_type("u8"))
    depth = 8;
  else if (type == babl_type("u16"))
    depth = 16;

  ret = FALSE;
  b = FALSE;

  do
    {
      components[0] = jas_image_getcmptbytype
        (p->image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_R));
      components[1] = jas_image_getcmptbytype
        (p->image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_G));
      components[2] = jas_image_getcmptbytype
        (p->image, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_RGB_B));

      if ((components[0] < 0) || (components[1] < 0) || (components[2] < 0))
        {
          g_warning ("%s",_("One or more of R, G, B components are missing"));
          break;
        }

      if (jas_image_cmptsgnd (p->image, components[0]) ||
          jas_image_cmptsgnd (p->image, components[1]) ||
          jas_image_cmptsgnd (p->image, components[2]))
        {
          g_warning ("%s",_("One or more of R, G, B components have signed data"));
          break;
        }

      for (i = 0; i < 3; i++)
        matrices[i] = jas_matrix_create(1, p->width);

      switch (depth)
        {
        case 16:
          data_s = (gushort *) g_malloc (p->width * 3 * sizeof (gushort));
          break;

        case 8:
          data_b = (guchar *) g_malloc (p->width * 3 * sizeof (guchar));
          break;

        default:
          g_warning ("%s: Programmer stupidity error", G_STRLOC);
          return FALSE;
        }

      for (row = 0; row < p->height; row++)
        {
          gint plane, col;
          jas_seqent_t *jrow[3] = {NULL, NULL, NULL};

          for (plane = 0; plane < 3; plane++)
            {
              int r = jas_image_readcmpt (p->image, components[plane], 0, row,
                                          p->width, 1, matrices[plane]);
              if (r)
                {
                  g_warning (_("Error reading row %d component %d"), row, plane);
                  b = TRUE;
                  break;
                }
            }

          if (b)
            break;

          for (plane = 0; plane < 3; plane++)
            jrow[plane] = jas_matrix_getref (matrices[plane], 0, 0);

          switch (depth)
            {
            case 16:
              for (col = 0; col < p->width; col++)
                {
                  data_s[col * 3]     = (gushort) jrow[0][col];
                  data_s[col * 3 + 1] = (gushort) jrow[1][col];
                  data_s[col * 3 + 2] = (gushort) jrow[2][col];
                }
              break;

            case 8:
              for (col = 0; col < p->width; col++)
                {
                  data_b[col * 3]     = (guchar) jrow[0][col];
                  data_b[col * 3 + 1] = (guchar) jrow[1][col];
                  data_b[col * 3 + 2] = (guchar) jrow[2][col];
                }
              break;

            default:
              g_warning ("%s: Programmer stupidity error", G_STRLOC);
              b = TRUE;
            }

          if (b)
            break;

          rect.x = 0;
          rect.y = row;
          rect.width = p->width;
          rect.height = 1;

          switch (depth)
            {
            case 16:
              gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u16"),
                               data_s, GEGL_AUTO_ROWSTRIDE);
              break;

            case 8:
              gegl_buffer_set (output, &rect, 0, babl_format ("R'G'B' u8"),
                               data_b, GEGL_AUTO_ROWSTRIDE);
	      break;

            default:
              g_warning ("%s: Programmer stupidity error", G_STRLOC);
              b = TRUE;
            }
        }

      if (b)
        break;

      ret = TRUE;
    }
  while (FALSE); /* structured goto */

  for (i = 0; i < 3; i++)
    if (matrices[i])
      jas_matrix_destroy (matrices[i]);

  if (data_b)
    g_free (data_b);

  if (data_s)
    g_free (data_s);

  return ret;
}

static GeglRectangle
get_bounding_box (GeglOperation * operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  Priv *p = (Priv*) o->user_data;

  if (p->image != NULL)
    {
      result.width = p->width;
      result.height = p->height;
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
finalize(GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES(object);

  if (o->user_data != NULL)
    {
      cleanup(GEGL_OPERATION(object));
      if (o->user_data != NULL)
        g_free(o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS(gegl_op_parent_class)->finalize(object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:jp2-load",
    "title",       _("JPEG 2000 File Loader"),
    "categories",  "hidden",
    "description", _("JPEG 2000 image loader using jasper."),
    NULL);

  gegl_operation_handlers_register_loader (
    "image/jp2", "gegl:jp2-load");
  gegl_operation_handlers_register_loader (
    ".jp2", "gegl:jp2-load");
  gegl_operation_handlers_register_loader (
    ".jpf", "gegl:jp2-load");
  gegl_operation_handlers_register_loader (
    ".jpx", "gegl:jp2-load");
}

#endif
