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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_path   (path,   _("Path"),
                             _("A GeglPath representing the path of the fill"))
gegl_chant_color  (color,    _("Color"),      "rgba(0.1,0.2,0.3,1.0)",
                             _("Color of paint to use"))
gegl_chant_boolean(winding,  _("Winding"),    TRUE,
                             _("what kind of fill rule to use"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE "fill.c"

#include "gegl-plugin.h"
#include "property-types/gegl-path.h"

#include "gegl-chant.h"
#include <cairo/cairo.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("B'aG'aR'aA u8"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  gdouble        x0, x1, y0, y1;

  gegl_path_get_bounds (o->path, &x0, &x1, &y0, &y1);
  defined.x      = x0;
  defined.y      = y0;
  defined.width  = x1 - x0;
  defined.height = y1 - y0;

  return defined;
}


static void foreach_cairo (const GeglPathItem *knot,
                           gpointer              cr)
{
  switch (knot->type)
    {
      case 'M':
        cairo_move_to (cr, knot->point[0].x, knot->point[0].y);
        break;
      case 'L':
        cairo_line_to (cr, knot->point[0].x, knot->point[0].y);
        break;
      case 'C':
        cairo_curve_to (cr, knot->point[0].x, knot->point[0].y,
                            knot->point[1].x, knot->point[1].y,
                            knot->point[2].x, knot->point[2].y);
        break;
      case 'z':
        cairo_close_path (cr);
        break;
      default:
        g_print ("%s uh?:%c\n", G_STRLOC, knot->type);
    }
}

static void gegl_path_cairo_play (GeglPath *path,
                                    cairo_t *cr)
{
  gegl_path_foreach_flat (path, foreach_cairo, cr);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  cairo_t *cr;
  cairo_surface_t *surface;
  gfloat r,g,b,a;
  guchar *data = (void*)gegl_buffer_linear_open (output, result, NULL, babl_format ("B'aG'aR'aA u8"));

  surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 result->width,
                                                 result->height,
                                                 result->width * 4);
  memset (data, 0, result->width * result->height * 4);
  cr = cairo_create (surface);
  cairo_translate (cr, -result->x, -result->y);

  gegl_path_cairo_play (o->path, cr);
  gegl_color_get_rgba (o->color, &r,&g,&b,&a);
  cairo_set_source_rgba (cr, r,g,b,a);
  cairo_fill (cr);
  gegl_buffer_linear_close (output, data);
  return  TRUE;
}

static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  cairo_t *cr;
  cairo_surface_t *surface;
  gchar *data = "     ";
  gboolean result;

  surface = cairo_image_surface_create_for_data ((guchar*)data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 1,1,4);
  cr = cairo_create (surface);
  /*cairo_translate (cr, -result->x, -result->y);*/
  gegl_path_cairo_play (o->path, cr);
  result = cairo_in_fill (cr, x, y);
  cairo_destroy (cr);

  if (result)
    return operation->node;

  return NULL;
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
  operation_class->prepare = prepare;
  operation_class->detect = detect;

  operation_class->name        = "gegl:fill";
  operation_class->categories  = "render";
  operation_class->description = _("Renders a fill of the provided GeglPath in a given color");

 /* operation_class->get_cached_region = get_cached_region;*/
}


#endif
