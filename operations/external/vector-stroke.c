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


gegl_chant_color  (color,    _("Color"),      "rgba(0.0,0.0,0.0,0.0)",
                             _("Color of paint to use for stroking."))

gegl_chant_double (width,    _("Width"),  0.0, 200.0, 2.0,
                             _("The width of the brush used to stroke the path."))

gegl_chant_double (opacity,  _("Opacity"),  -2.0, 2.0, 1.0,
                             _("Opacity of stroke, note, does not behave like SVG since at the moment stroking is done using an airbrush tool."))

gegl_chant_string (transform,_("Transform"), "",
                             _("svg style description of transform."))

gegl_chant_path   (d,        _("Vector"),
                             _("A GeglVector representing the path of the stroke"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE "vector-stroke.c"

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
  GeglRectangle rect = *roi;
  GeglChantO    *o   = GEGL_CHANT_PROPERTIES (userdata);
  /* invalidate the incoming rectangle */

  rect.x -= o->width/2;
  rect.y -= o->width/2;
  rect.width += o->width;
  rect.height += o->width;

  gegl_operation_invalidate (userdata, &rect, FALSE);
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
  defined.x      = x0 - o->width/2;
  defined.y      = y0 - o->width/2;
  defined.width  = x1 - x0 + o->width;
  defined.height = y1 - y0 + o->width;

  if (in_rect)
    {
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}



#if 0
static gboolean gegl_path_is_closed (GeglPath *path)
{
  const GeglPathItem *knot;

  if (!path)
    return FALSE;
  knot = gegl_path_get_node (path, -1);
  if (!knot)
    return FALSE;
  if (knot->type == 'z')
    {
      return TRUE;
    }
  return FALSE;
}
#endif


#if 0
static GeglRectangle
get_cached_region (GeglOperation *operation)
{
  return get_bounding_box (operation);
}
#endif
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
  gboolean need_stroke = FALSE;
  gdouble r,g,b,a;

  if (input)
    {
      gegl_buffer_copy (input, result, output, result);
    }
  else
    {
      gegl_buffer_clear (output, result);
    }

  if (o->width > 0.1 && o->opacity > 0.0001)
    {
      gegl_color_get_rgba (o->color, &r,&g,&b,&a);
      a *= o->opacity;
      if (a>0.001)
          need_stroke=TRUE;
    }

  if (need_stroke)
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

      cairo_set_line_width  (cr, o->width);
      cairo_set_line_cap    (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join   (cr, CAIRO_LINE_JOIN_ROUND);

      gegl_path_cairo_play (o->d, cr);
      cairo_set_source_rgba (cr, r,g,b,a);
      cairo_stroke (cr);
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
  cairo_set_line_width (cr, o->width);


  if (o->width > 0.1 && o->opacity > 0.0001)
    result = cairo_in_stroke (cr, x, y);

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
  /*operation_class->no_cache = TRUE;*/

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:vector-stroke",
    "categories"  , "render",
    "description" , _("Renders a vector stroke"),
    NULL);
}


#endif
