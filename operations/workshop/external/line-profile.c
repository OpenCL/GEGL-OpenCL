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
 * Copyright 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (x0, _("X0"), 0, 1000, 0, _("Start x coordinate"))
gegl_chant_int (x1, _("X1"), 0, 1000, 200, _("End x coordinate"))
gegl_chant_int (y0, _("Y0"), 0, 1000, 0, _("Start y coordinate"))
gegl_chant_int (y1, _("Y1"), 0, 1000, 200, _("End y coordinate"))
gegl_chant_int (width,  _("Width"),  10, 10000, 1024, _("Width of plot"))
gegl_chant_int (height, _("Height"), 10, 10000, 256,  _("Height of plot"))
gegl_chant_double (min, _("Min"), -500.0, 500,  0.0, _("Value at bottom"))
gegl_chant_double (max, _("Max"), -500.0, 500,  8.0, _("Value at top"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "line-profile.c"

#include "gegl-chant.h"
#include <cairo.h>

static gfloat
buffer_sample (GeglBuffer *buffer,
               gint        x,
               gint        y,
               gint        component)
{
  gfloat rgba[4];
  GeglRectangle roi = {x,y,1,1};

  gegl_buffer_get (buffer, 1.0, &roi, babl_format ("RGBA float"), &rgba[0], GEGL_AUTO_ROWSTRIDE);
  return rgba[component];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gint        width = MAX(MAX (o->width, o->x0), o->x1);
  gint        height = MAX(MAX (o->height, o->y0), o->y1);

  {
    GeglRectangle extent = {0,0,width,height};
    output = gegl_buffer_new (&extent, babl_format ("B'aG'aR'aA u8"));
  }

  {
    guchar  *buf = g_new0 (guchar, width * height * 4);
    cairo_t *cr;

    cairo_surface_t *surface = cairo_image_surface_create_for_data (buf, CAIRO_FORMAT_ARGB32, width, height, width * 4);
    cr = cairo_create (surface);
  /*  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle (cr, 0,0, o->width, o->height);
    cairo_fill (cr);*/

#define val2y(val) (o->height - (val - o->min) * o->height / (o->max-o->min))

    cairo_set_source_rgba (cr, .0, .0, .8, 0.5);
    cairo_move_to (cr, 0, val2y(0.0));
    cairo_line_to (cr, o->width, val2y(0.0));

    cairo_set_source_rgba (cr, .8, .8, .0, 0.5);
    cairo_move_to (cr, 0, val2y(1.0));
    cairo_line_to (cr, o->width, val2y(1.0));

    cairo_stroke (cr);

    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    {
      gint x;
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
      for (x=0;x<o->width;x++)
        {
          gfloat t = (1.0*x)/o->width;
          gint sx = ((1.0-t) * o->x0) + (t * o->x1);
          gint sy = ((1.0-t) * o->y0) + (t * o->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,0)));
        }
      cairo_stroke (cr);
    }
    {
      gint x;
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
      for (x=0;x<o->width;x++)
        {
          gfloat t = (1.0*x)/o->width;
          gint sx = ((1.0-t) * o->x0) + (t * o->x1);
          gint sy = ((1.0-t) * o->y0) + (t * o->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,1)));
        }
      cairo_stroke (cr);
    }
    {
      gint x;
      cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 1.0);
      for (x=0;x<o->width;x++)
        {
          gfloat t = (1.0*x)/o->width;
          gint sx = ((1.0-t) * o->x0) + (t * o->x1);
          gint sy = ((1.0-t) * o->y0) + (t * o->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,2)));
        }
      cairo_stroke (cr);
    }
   cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.4);
   cairo_move_to (cr, o->x0, o->y0);
   cairo_line_to (cr, o->x1, o->y1);
   cairo_stroke (cr);

    gegl_buffer_set (output, NULL, babl_format ("B'aG'aR'aA u8"), buf, GEGL_AUTO_ROWSTRIDE);
  }

  return TRUE;
}

static GeglRectangle
get_required_for_output (GeglOperation        *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle defined = {0,0,o->width,o->height};

  defined.width  = MAX (MAX (o->width,  o->x0), o->x1);
  defined.height = MAX (MAX (o->height, o->y0), o->y1);
  return defined;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->name        = "gegl:line-profile";
  operation_class->categories  = "debug";
  operation_class->description =
        _("Renders luminance profiles for red green and blue components along"
          " the specified line in the input buffer, plotted in a buffer of the"
          " specified size.");
}

#endif
