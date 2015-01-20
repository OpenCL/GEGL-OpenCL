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
    description    (_("Path to SVG file to load"))

property_int (width,  _("Width"),  -1)
    description (_("Width for rendered image"))

property_int (height, _("Height"), -1)
    description (_("Height for rendered image"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE svg-load.c

#include "gegl-op.h"
#include <cairo.h>
#include <librsvg/rsvg.h>

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A u8"));
}

static gint
gegl_buffer_import_svg (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         width,
                        gint         height)
{
    cairo_surface_t   *surface;
    cairo_t           *cr;
    GError            *error = NULL;
    RsvgDimensionData  svg_dimentions;
    RsvgHandle        *handle;

    handle = rsvg_handle_new_from_file (path, &error);

    if (!handle)
      return 1;

    rsvg_handle_get_dimensions (handle, &svg_dimentions);

    if (svg_dimentions.width == 0 || svg_dimentions.height == 0)
      return 0;

    if (width < 1)
      width = svg_dimentions.width;

    if (height < 1)
      height = svg_dimentions.height;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);

    if (width  != svg_dimentions.width ||
        height != svg_dimentions.height)
      {
        cairo_scale (cr,
                     (double)width / (double)svg_dimentions.width,
                     (double)height / (double)svg_dimentions.height);
      }

    rsvg_handle_render_cairo (handle, cr);

    cairo_surface_flush (surface);

    gegl_buffer_set (gegl_buffer,
                     GEGL_RECTANGLE (0, 0, width, height),
                     0,
                     babl_format ("cairo-ARGB32"),
                     cairo_image_surface_get_data (surface),
                     cairo_image_surface_get_stride (surface));

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    g_object_unref (handle);

    return 0;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o      = GEGL_PROPERTIES (operation);
  gint          width  = o->width;
  gint          height = o->height;

  if (!o->path || !strlen(o->path))
    return *GEGL_RECTANGLE(0, 0, 0, 0);

  if (width < 1 || height < 1)
    {
      RsvgDimensionData  svg_dimentions;
      RsvgHandle        *handle;
      GError            *error = NULL;

      handle = rsvg_handle_new_from_file (o->path, &error);

      if (!handle)
        return *GEGL_RECTANGLE(0, 0, 0, 0);

      rsvg_handle_get_dimensions (handle, &svg_dimentions);

      if (width < 1)
        width = svg_dimentions.width;

      if (height < 1)
        height = svg_dimentions.height;

      rsvg_handle_get_dimensions (handle, &svg_dimentions);
    }

  return *GEGL_RECTANGLE(0, 0, width, height);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result_foo,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gint        result;
  gint        width, height;

  width  = o->width;
  height = o->height;

  result = gegl_buffer_import_svg (output, o->path, width, height);
  if (result)
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);
      return  FALSE;
    }

  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:svg-load",
    "title",        _("SVG File Loader"),
    "categories"  , "input",   /* not hidden because it has extra API */
    "description" , _("Load an SVG file using librsvg"),
    NULL);

  gegl_extension_handler_register (".svg", "gegl:svg-load");
  gegl_extension_handler_register (".svgz", "gegl:svg-load");
}

#endif
