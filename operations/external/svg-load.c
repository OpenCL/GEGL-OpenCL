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
 * Copyright 2006 Kevin Cozens <kcozens@cvs.gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI for file to load"))

property_int (width, _("Width"), -1)
  description (_("Width for rendered image"))
property_int (height, _("Height"), -1)
  description (_("Height for rendered image"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE svg-load.c

#include <gegl-op.h>
#include <cairo.h>
#include <gegl-gio-private.h>
#include <librsvg/rsvg.h>

typedef struct
{
  GFile *file;

  RsvgHandle *handle;

  const Babl *format;

  gint width;
  gint height;
} Priv;

static void
cleanup (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      if (p->handle != NULL)
        g_clear_object (&p->handle);

      if (p->file != NULL)
        g_clear_object(&p->file);

      p->width = p->height = 0;
      p->format = NULL;
    }
}

static gboolean
query_svg (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;
  RsvgDimensionData dimentions;

  g_return_val_if_fail (p->handle != NULL, FALSE);

  rsvg_handle_get_dimensions (p->handle, &dimentions);

  p->format = babl_format ("R'G'B'A u8");

  p->height = dimentions.height;
  p->width = dimentions.width;

  return TRUE;
}

static gint
load_svg (GeglOperation *operation,
          GeglBuffer    *output,
          gint           width,
          gint           height)
{
    GeglProperties    *o = GEGL_PROPERTIES (operation);
    Priv              *p = (Priv*) o->user_data;
    cairo_surface_t   *surface;
    cairo_t           *cr;

    g_return_val_if_fail (p->handle != NULL, -1);

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);

    if (width != p->width || height != p->height)
      {
        cairo_scale (cr,
                     (double)width / (double)p->width,
                     (double)height / (double)p->height);
      }

    rsvg_handle_render_cairo (p->handle, cr);

    cairo_surface_flush (surface);

    gegl_buffer_set (output,
                     GEGL_RECTANGLE (0, 0, width, height),
                     0,
                     babl_format ("cairo-ARGB32"),
                     cairo_image_surface_get_data (surface),
                     cairo_image_surface_get_stride (surface));

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return 0;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (o->user_data) ? o->user_data : g_new0 (Priv, 1);
  GError *error = NULL;
  GInputStream *stream;
  GFile *file = NULL;

  g_assert (p != NULL);

  if (p->file != NULL && (o->uri || o->path))
    {
      if (o->uri && strlen (o->uri) > 0)
        file = g_file_new_for_uri (o->uri);
      else if (o->path && strlen (o->path) > 0)
        file = g_file_new_for_path (o->path);
      if (file != NULL)
        {
          if (!g_file_equal (p->file, file))
            cleanup (operation);
          g_object_unref (file);
        }
    }

  o->user_data = (void*) p;

  if (p->handle == NULL)
    {
      stream = gegl_gio_open_input_stream (o->uri, o->path, &p->file, &error);
      if (stream == NULL)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
          cleanup (operation);
          return;
        }

      p->handle = rsvg_handle_new_from_stream_sync (stream, p->file,
                                                    RSVG_HANDLE_FLAGS_NONE,
                                                    NULL, &error);
      if (p->handle == NULL)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
          cleanup (operation);
          return;
        }

      if (!query_svg (operation))
        {
          g_warning ("could not query SVG image file");
          cleanup (operation);
          return;
        }
    }

  gegl_operation_set_format (operation, "output", p->format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  Priv *p = (Priv*) o->user_data;
  gint width = o->width;
  gint height = o->height;

  if (p->handle != NULL)
    {
      if (width < 1)
        width = p->width;
      if (height < 1)
        height = p->height;

      result.width = width;
      result.height = height;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;
  gint width = o->width;
  gint height = o->height;

  if (p->handle != NULL)
  {
    if (width < 1)
      width = p->width;
    if (height < 1)
      height = p->height;

    if (load_svg (operation, output, width, height))
      {
        if (o->uri != NULL && strlen(o->uri) > 0)
          g_warning ("failed to render SVG from %s", o->uri);
        else
          g_warning ("failed to render SVG from %s", o->path);
        return FALSE;
      }

    return TRUE;
  }

  return FALSE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data != NULL)
    {
      cleanup (GEGL_OPERATION (object));
      if (o->user_data != NULL)
        g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS(klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:svg-load",
    "title",        _("SVG File Loader"),
    "categories"  , "input",   /* not hidden because it has extra API */
    "description" , _("Load an SVG file using librsvg"),
    NULL);

  gegl_operation_handlers_register_loader (
    "image/svg+xml", "gegl:svg-load");
  gegl_operation_handlers_register_loader (
    ".svg", "gegl:svg-load");

  gegl_operation_handlers_register_loader (
    "image/svg+xml-compressed", "gegl:svg-load");
  gegl_operation_handlers_register_loader (
    ".svgz", "gegl:svg-load");
}

#endif
