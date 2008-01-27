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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_path (path, "/tmp/test.svg", "Path to SVG file to load")
gegl_chant_int (width,  0, G_MAXINT, 100, "Width for rendered image")
gegl_chant_int (height, 0, G_MAXINT, 100, "Height for rendered image")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "svg-load.c"

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
      guchar        *pixeldata;
      GeglRectangle  rect = {dest_x, dest_y, width, height};

      pixeldata = gdk_pixbuf_get_pixels (pixbuf);
      gegl_buffer_set (gegl_buffer, &rect, babl_format ("R'G'B'A u8"), pixeldata, GEGL_AUTO_ROWSTRIDE);
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
  if (handle == NULL)
      return FALSE;

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

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};
  /*GeglOperationSource *source = GEGL_OPERATION_SOURCE(operation);*/
  gint width, height;
  gint status;

  /*if (!strcmp (o->path, "-"))
    {
      process (operation);
      width = source->output->width;
      height = source->output->height;
    }
  else*/
    {
      width  = o->width;
      height = o->height;
      status = query_svg (o->path, &width, &height);
      if (status == FALSE)
        {
          g_warning ("get defined region of %s failed", o->path);
          width = 0;
          height = 0;
        }
    }

  result.width  = width;
  result.height  = height;
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *output,
         const GeglRectangle *result_foo)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gint        result;
  gint        width, height;

    width  = o->width;
    height = o->height;
    result = query_svg (o->path, &width, &height);
    if (result == FALSE)
      {
        g_warning ("%s failed to open file %s for reading.",
          G_OBJECT_TYPE_NAME (operation), o->path);
          output = gegl_buffer_new (NULL, NULL);
            return TRUE;
      }

      {
        GeglRectangle extent={0,0,width,height};
        /* FIXME: the format should be set on the pad prior to the actual
         * load
         */
        output = gegl_buffer_new (&extent, babl_format ("R'G'B'A u8"));
      }

  result = gegl_buffer_import_svg (output, o->path,
                                   width, height, 0, 0, &width, &height);
  if (result)
    {
      g_warning ("%s failed to open file %s for reading.",
        G_OBJECT_TYPE_NAME (operation), o->path);
      return  FALSE;
    }
  gegl_node_context_set_object (context, "output", G_OBJECT (output));

  return  TRUE;
}


static void
operation_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_defined_region = get_defined_region;

  operation_class->name        = "svg-load";
  operation_class->categories  = "input";   /* not hidden because it has extra API */
  operation_class->description = "Load an SVG file using librsvg";

//  static gboolean done=FALSE;
//  if (done)
//    return;
  gegl_extension_handler_register (".svg", "svg-load");
  gegl_extension_handler_register (".SVG", "svg-load");
  gegl_extension_handler_register (".svgz", "svg-load");
  gegl_extension_handler_register (".SVGZ", "svg-load");
//  done = TRUE;
}

#endif
