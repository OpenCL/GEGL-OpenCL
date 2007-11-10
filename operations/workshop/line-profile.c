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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_int (x0, 0, 1000, 0, "start x coordinate")
gegl_chant_int (x1, 0, 1000, 200, "end x coordinate")
gegl_chant_int (y0, 0, 1000, 0, "start y coordinate")
gegl_chant_int (y1, 0, 1000, 200, "end y coordinate")
gegl_chant_int (width, 10, 10000, 1024, "width of plot")
gegl_chant_int (height, 10, 10000, 256, "height of plot")
gegl_chant_double (min, -500.0, 500,  0.0, "value at bottom")
gegl_chant_double (max, -500.0, 500,  8.0, "value at top")
 
#else

#define GEGL_CHANT_NAME            line_profile
#define GEGL_CHANT_SELF            "line-profile.c"
#define GEGL_CHANT_DESCRIPTION     "Renders luminance profiles for red green and blue components along the specified line in the input buffer, plotted in a buffer of the specified size."
#define GEGL_CHANT_CATEGORIES      "debug"

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_CLASS_INIT

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
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglRectangle *result;
  GeglBuffer    *input,
                *output;
  gint width = MAX(MAX (self->width, self->x0), self->x1);
  gint height = MAX(MAX (self->height, self->y0), self->y1);

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));

  result = gegl_operation_get_requested_region (operation, context_id);
 
  { 
    GeglRectangle extent = {0,0,width,height};
    output = gegl_buffer_new (&extent, babl_format ("B'aG'aR'aA u8"));
  }

  {
    guchar  *buf = g_malloc0 (width * height * 4);
    cairo_t *cr;

    cairo_surface_t *surface = cairo_image_surface_create_for_data (buf, CAIRO_FORMAT_ARGB32, width, height, width * 4);
    cr = cairo_create (surface);
  /*  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle (cr, 0,0, self->width, self->height);
    cairo_fill (cr);*/

#define val2y(val) (self->height - (val - self->min) * self->height / (self->max-self->min))

    cairo_set_source_rgba (cr, .0, .0, .8, 0.5);
    cairo_move_to (cr, 0, val2y(0.0));
    cairo_line_to (cr, self->width, val2y(0.0));

    cairo_set_source_rgba (cr, .8, .8, .0, 0.5);
    cairo_move_to (cr, 0, val2y(1.0));
    cairo_line_to (cr, self->width, val2y(1.0));

    cairo_stroke (cr);

    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    {
      gint x;
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
      for (x=0;x<self->width;x++)
        {
          gfloat t = (1.0*x)/self->width;
          gint sx = ((1.0-t) * self->x0) + (t * self->x1);
          gint sy = ((1.0-t) * self->y0) + (t * self->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,0)));
        }
      cairo_stroke (cr);
    }
    {
      gint x;
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
      for (x=0;x<self->width;x++)
        {
          gfloat t = (1.0*x)/self->width;
          gint sx = ((1.0-t) * self->x0) + (t * self->x1);
          gint sy = ((1.0-t) * self->y0) + (t * self->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,1)));
        }
      cairo_stroke (cr);
    }
    {
      gint x;
      cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 1.0);
      for (x=0;x<self->width;x++)
        {
          gfloat t = (1.0*x)/self->width;
          gint sx = ((1.0-t) * self->x0) + (t * self->x1);
          gint sy = ((1.0-t) * self->y0) + (t * self->y1);
          cairo_line_to (cr, x, val2y(buffer_sample(input,sx,sy,2)));
        }
      cairo_stroke (cr);
    }
   cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.4);
   cairo_move_to (cr, self->x0, self->y0);
   cairo_line_to (cr, self->x1, self->y1);
   cairo_stroke (cr);

    gegl_buffer_set (output, NULL, babl_format ("B'aG'aR'aA u8"), buf);
  }

  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));

  return TRUE;
}

static GeglRectangle
compute_input_request (GeglOperation *self,
                       const gchar   *input_pad,
                       GeglRectangle *roi)
{
  return *gegl_operation_source_get_defined_region (self, "input");
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglRectangle defined = {0,0,self->width,self->height};

  defined.width = MAX(MAX (self->width, self->x0), self->x1);
  defined.height = MAX(MAX (self->height, self->y0), self->y1);
  return defined;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->compute_input_request = compute_input_request;
  operation_class->get_defined_region = get_defined_region;
}

#endif
