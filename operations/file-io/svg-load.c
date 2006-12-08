/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Kevin Cozens <kcozens@cvs.gimp.org>
 */
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, "/tmp/test.svg", "Path to SVG file to load")
gegl_chant_int (width,  0, G_MAXINT, 100, "Width for rendered image")
gegl_chant_int (height, 0, G_MAXINT, 100, "Height for rendered image")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME            svg_load
#define GEGL_CHANT_DESCRIPTION     "Load an SVG file using librsvg"
#define GEGL_CHANT_SELF            "svg-load.c"
#define GEGL_CHANT_CATEGORIES      "input" /* not hidden because it has extra API */
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

#include <cairo.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#define SVG_DEFAULT_RESOLUTION  90.0
#define SVG_DEFAULT_SIZE        500

typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
} SvgLoadVals;


static gint
gegl_buffer_import_svg (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         width,
                        gint         height,
                        gint         dest_x,
                        gint         dest_y,
                        gint        *ret_width,
                        gint        *ret_height);

gint query_svg (const gchar *path,
                gint        *width,
                gint        *height);

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglChantOperation       *self = GEGL_CHANT_OPERATION (operation);
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  gint                 result;

  {
    gint width, height;

      width  = self->width;
      height = self->height;
      result = query_svg (self->path, &width, &height);
      if (result == FALSE)
        {
          g_warning ("%s failed to open file %s for reading.",
            G_OBJECT_TYPE_NAME (operation), self->path);
          op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                            "format", babl_format ("R'G'B'A u8"),
                                            "x",      0,
                                            "y",      0,
                                            "width",  10,
                                            "height", 10,
                                            NULL);
              return TRUE;
        }

      op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                                        "format", babl_format ("R'G'B'A u8"),
                                        "x",      0,
                                        "y",      0,
                                        "width",  width,
                                        "height", height,
                                        NULL);

    result = gegl_buffer_import_svg (op_source->output, self->path,
                                     width, height, 0, 0, &width, &height);
    if (result)
      {
        g_warning ("%s failed to open file %s for reading.",
          G_OBJECT_TYPE_NAME (operation), self->path);
        op_source->output = NULL;
        return  FALSE;
      }
  }

  return  TRUE;
}


static GeglRect
get_defined_region (GeglOperation *operation)
{
  GeglRect result = {0,0,0,0};
  GeglChantOperation    *self = GEGL_CHANT_OPERATION (operation);
  /*GeglOperationSource *source = GEGL_OPERATION_SOURCE(operation);*/
  gint width, height;
  gint status;

  /*if (!strcmp (self->path, "-"))
    {
      process (operation);
      width = source->output->width;
      height = source->output->height;
    }
  else*/
    {
      width  = self->width;
      height = self->height;
      status = query_svg (self->path, &width, &height);
      if (status == FALSE)
        {
          g_warning ("get defined region of %s failed", self->path);
          width = 0;
          height = 0;
        }
    }

  result.w = width;
  result.h = height;
  return result;
}

static gint
gegl_buffer_import_svg (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         width,
                        gint         height,
                        gint         dest_x,
                        gint         dest_y,
                        gint        *ret_width,
                        gint        *ret_height)
{
    cairo_surface_t *surface;
    cairo_t    *cr;
    GdkPixbuf  *pixbuf;
    GError     *pError = NULL;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, *ret_width, *ret_height);
    cr = cairo_create (surface);

    rsvg_init();

/*
FIXME: The routine 'rsvg_pixbuf_from_file_at_size' is deprecated. Set up a
cairo matrix and use rsvg_handle_new_from_file() + rsvg_handle_render_cairo()
instead.
*/
    pixbuf = rsvg_pixbuf_from_file_at_size (path,
                                            width,
                                            height,
                                            &pError);
    if (pixbuf)
    {
      guchar   *pixeldata;
      GeglRect  rect = {dest_x, dest_y, width, height};

      pixeldata = gdk_pixbuf_get_pixels (pixbuf);
      gegl_buffer_set (gegl_buffer, &rect, pixeldata, babl_format ("R'G'B'A u8"));
    }

    rsvg_term();

    cairo_destroy (cr);
    cairo_surface_destroy (surface);

    return 0;
}

/*  This is the callback used from load_rsvg_size().  */
static void
load_get_size_callback (gint     *width,
                        gint     *height,
                        gpointer  data)
{
  SvgLoadVals *vals = data;

  *width  = vals->width;
  *height = vals->height;

  if (*width < 1 || *height < 1)
    {
      *width  = SVG_DEFAULT_SIZE;
      *height = SVG_DEFAULT_SIZE;
    }

  /*  cancel loading  */
  vals->resolution = 0.0;
}

gint
query_svg (const gchar *path,
           gint        *width,
           gint        *height)
{
  RsvgHandle       *handle;
  RsvgDimensionData dimension_data;
  GError           *pError = NULL;
  SvgLoadVals       vals;

  rsvg_init ();
  handle = rsvg_handle_new_from_file (path, &pError);

  vals.resolution = SVG_DEFAULT_RESOLUTION;
  vals.width  = *width;
  vals.height = *height;

  rsvg_handle_set_size_callback (handle, load_get_size_callback, &vals, NULL);

  rsvg_handle_get_dimensions (handle, &dimension_data);

  rsvg_handle_free (handle);
  rsvg_term ();

  *width  = dimension_data.width;
  *height = dimension_data.height;

  return TRUE;
}

static void class_init (GeglOperationClass *operation_class)
{
  static gboolean done=FALSE;

  if (done)
    return;

  gegl_extension_handler_register (".svg", "svg-load");
  gegl_extension_handler_register (".SVG", "svg-load");
  gegl_extension_handler_register (".svgz", "svg-load");
  gegl_extension_handler_register (".SVGZ", "svg-load");
  done = TRUE;
}

#endif
