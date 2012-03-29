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


gegl_chant_color  (color,    _("Color"),  "rgba(0.0,0.0,0.0,0.6)",
                             _("Color of paint to use for filling."))

gegl_chant_double (opacity,  _("Opacity"),  -2.0, 2.0, 1.0,
                             _("The fill opacity to use."))

gegl_chant_string (fill_rule,_("Fill rule."), "nonzero",
                             _("how to determine what to fill (nonzero|evenodd"))

gegl_chant_string (transform,_("Transform"), "",
                             _("svg style description of transform."))

gegl_chant_path   (d,        _("Vector"),
                             _("A GeglVector representing the path of the stroke"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE "vector-fill.c"

#include "gegl-plugin.h"

/* the path api isn't public yet */
#include "property-types/gegl-path.h"
static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata);

#include "gegl-chant.h"
#include <cairo.h>

static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata)
{
  GeglChantO    *o   = GEGL_CHANT_PROPERTIES (userdata);
  GeglRectangle rect;
  gdouble        x0, x1, y0, y1;

  gegl_path_get_bounds(o->d, &x0, &x1, &y0, &y1);
  rect.x = x0 - 1;
  rect.y = y0 - 1;
  rect.width = x1 - x0 + 2;
  rect.height = y1 - y0 + 2;

  gegl_operation_invalidate (userdata, &rect, TRUE);
};

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
  if (o->transform && o->transform[0] != '\0')
    {
      GeglMatrix3 matrix;
      gegl_matrix3_parse_string (&matrix, o->transform);
      gegl_path_set_matrix (o->d, &matrix);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;
  gdouble        x0, x1, y0, y1;

  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");

  gegl_path_get_bounds (o->d, &x0, &x1, &y0, &y1);
  defined.x      = x0;
  defined.y      = y0;
  defined.width  = x1 - x0;
  defined.height = y1 - y0;

  if (in_rect)
    {
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}

static void gegl_path_cairo_play (GeglPath *path,
                                    cairo_t *cr);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gboolean need_fill = FALSE;
  gdouble r,g,b,a;

  if (input)
    {
      gegl_buffer_copy (input, result, output, result);
    }
  else
    {
      gegl_buffer_clear (output, result);
    }


  if (o->opacity > 0.0001 && o->color)
    {
      gegl_color_get_rgba (o->color, &r,&g,&b,&a);
      a *= o->opacity;
      if (a>0.001)
          need_fill=TRUE;
    }

  if (need_fill)
    {
      GStaticMutex mutex = G_STATIC_MUTEX_INIT;
      cairo_t *cr;
      cairo_surface_t *surface;
      guchar *data;


      g_static_mutex_lock (&mutex);
      data = (void*)gegl_buffer_linear_open (output, result, NULL, babl_format ("B'aG'aR'aA u8"));
      surface = cairo_image_surface_create_for_data (data,
                                                     CAIRO_FORMAT_ARGB32,
                                                     result->width,
                                                     result->height,
                                                     result->width * 4);

      cr = cairo_create (surface);
      cairo_translate (cr, -result->x, -result->y);
      if (g_str_equal (o->fill_rule, "evenodd"))
          cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

      gegl_path_cairo_play (o->d, cr);
      cairo_set_source_rgba (cr, r,g,b,a);
      cairo_fill (cr);
      cairo_destroy (cr);

      gegl_buffer_linear_close (output, data);
      g_static_mutex_unlock (&mutex);
    }
  return  TRUE;
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

static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  cairo_t *cr;
  cairo_surface_t *surface;
  gchar *data = "     ";
  gboolean result = FALSE;

  surface = cairo_image_surface_create_for_data ((guchar*)data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 1,1,4);
  cr = cairo_create (surface);
  gegl_path_cairo_play (o->d, cr);

  if (!result)
    {
      if (o->d)
        {
          gdouble r,g,b,a;
          gegl_color_get_rgba (o->color, &r,&g,&b,&a);
          if (a * o->opacity>0.8)
            result = cairo_in_fill (cr, x, y);
        }
    }

  cairo_destroy (cr);

  if (result)
    return operation->node;

  return NULL;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->detect = detect;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:fill-path",
    "categories" , "render",
    "description", _("Renders a filled region"),
    NULL);
}


#endif
