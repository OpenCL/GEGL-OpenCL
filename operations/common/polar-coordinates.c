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
 * Polarize plug-in --- maps a rectangle to a circle or vice-versa
 * Copyright (C) 1997 Daniel Dunbar
 * Email: ddunbar@diads.com
 * WWW:   http://millennium.diads.com/gimp/
 * Copyright (C) 1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 * Copyright (C) 1996 Marc Bless
 * E-mail: bless@ai-lab.fh-furtwangen.de
 * WWW:    www.ai-lab.fh-furtwangen.de/~bless
 *
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (depth, _("Circle depth in percent"), 0.0, 100.0, 100.0,
                   _("Circle depth in percent"))
gegl_chant_double (angle, _("Offset angle"), 0.0, 359.9, 0.0,
                   _("Offset angle."))
gegl_chant_boolean (bw, _("Map backwards"), FALSE, _("Start from the right instead of the left"))
gegl_chant_boolean (top, _("Map from top"), TRUE, _("Put the top row in the middle and the bottom row on the outside"))
gegl_chant_boolean (polar, _("To polar"), TRUE, _("Map the image to a circle"))
gegl_chant_int (pole_x, _("X:"), 0, G_MAXINT, 0,
                _("Origin point for the polar coordinates"))
gegl_chant_int (pole_y, _("Y:"), 0, G_MAXINT, 0,
                _("Origin point for the polar coordinates"))
gegl_chant_boolean (middle, _("Choose middle"), TRUE,
                    _("Let origin point to be the middle one"))


#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "polar-coordinates.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)
#define SQR(x) (x)*(x)

#define SCALE_WIDTH     200
#define ENTRY_WIDTH      60

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
calc_undistorted_coords (gdouble        wx,
                         gdouble        wy,
                         gdouble       *x,
                         gdouble       *y,
                         GeglChantO    *o,
                         GeglRectangle  boundary)
{
  gboolean inside;
  gdouble  phi, phi2;
  gdouble  xx, xm, ym, yy;
  gint     xdiff, ydiff;
  gdouble  r;
  gdouble  m;
  gdouble  xmax, ymax, rmax;
  gdouble  x_calc, y_calc;
  gdouble  xi, yi, cen_x, cen_y;
  gdouble  circle, angl, t, angle;
  gint     x1, x2, y1, y2;

  /* initialize */

  phi = 0.0;
  r   = 0.0;

  x1     = 0;
  y1     = 0;
  x2     = boundary.width;
  y2     = boundary.height;
  xdiff  = x2 - x1;
  ydiff  = y2 - y1;
  xm     = xdiff / 2.0;
  ym     = ydiff / 2.0;
  circle = o->depth;
  angle  = o->angle;
  angl   = (gdouble) angle / 180.0 * G_PI;
  cen_x  = o->pole_x;
  cen_y  = o->pole_y;


  if (o->polar)
    {
      if (wx >= cen_x)
        {
          if (wy > cen_y)
            {
              phi = G_PI - atan (((double)(wx - cen_x))/
                                 ((double)(wy - cen_y)));
            }
          else if (wy < cen_y)
            {
              phi = atan (((double)(wx - cen_x))/((double)(cen_y - wy)));
            }
          else
            {
              phi = G_PI / 2;
            }
        }
      else if (wx < cen_x)
        {
          if (wy < cen_y)
            {
              phi = 2 * G_PI - atan (((double)(cen_x -wx)) /
                                     ((double)(cen_y - wy)));
            }
          else if (wy > cen_y)
            {
              phi = G_PI + atan (((double)(cen_x - wx))/
                                 ((double)(wy - cen_y)));
            }
          else
            {
              phi = 1.5 * G_PI;
            }
        }

      r   = sqrt (SQR (wx - cen_x) + SQR (wy - cen_y));

      if (wx != cen_x)
        {
          m = fabs (((double)(wy - cen_y)) / ((double)(wx - cen_x)));
        }
      else
        {
          m = 0;
        }

      if (m <= ((double)(y2 - y1) / (double)(x2 - x1)))
        {
          if (wx == cen_x)
            {
              xmax = 0;
              ymax = cen_y - y1;
            }
          else
            {
              xmax = cen_x - x1;
              ymax = m * xmax;
            }
        }
      else
        {
          ymax = cen_y - y1;
          xmax = ymax / m;
        }

      rmax = sqrt ( (double)(SQR (xmax) + SQR (ymax)) );

      t = ((cen_y - y1) < (cen_x - x1)) ? (cen_y - y1) : (cen_x - x1);
      rmax = (rmax - t) / 100 * (100 - circle) + t;

      phi = fmod (phi + angl, 2*G_PI);

      if (o->bw)
        x_calc = x2 - 1 - (x2 - x1 - 1)/(2*G_PI) * phi;
      else
        x_calc = (x2 - x1 - 1)/(2*G_PI) * phi + x1;

      if (o->top)
        y_calc = (y2 - y1)/rmax   * r   + y1;
      else
        y_calc = y2 - (y2 - y1)/rmax * r;
    }
  else
    {
      if (o->bw)
        phi = (2 * G_PI) * (x2 - wx) / xdiff;
      else
        phi = (2 * G_PI) * (wx - x1) / xdiff;

      phi = fmod (phi + angl, 2 * G_PI);

      if (phi >= 1.5 * G_PI)
        phi2 = 2 * G_PI - phi;
      else if (phi >= G_PI)
        phi2 = phi - G_PI;
      else if (phi >= 0.5 * G_PI)
        phi2 = G_PI - phi;
      else
        phi2 = phi;

      xx = tan (phi2);
      if (xx != 0)
        m = (double) 1.0 / xx;
      else
        m = 0;

      if (m <= ((double)(ydiff) / (double)(xdiff)))
        {
          if (phi2 == 0)
            {
              xmax = 0;
              ymax = ym - y1;
            }
          else
            {
              xmax = xm - x1;
              ymax = m * xmax;
            }
        }
      else
        {
          ymax = ym - y1;
          xmax = ymax / m;
        }

      rmax = sqrt ((double)(SQR (xmax) + SQR (ymax)));

      t = ((ym - y1) < (xm - x1)) ? (ym - y1) : (xm - x1);

      rmax = (rmax - t) / 100.0 * (100 - circle) + t;

      if (o->top)
        r = rmax * (double)((wy - y1) / (double)(ydiff));
      else
        r = rmax * (double)((y2 - wy) / (double)(ydiff));

      xx = r * sin (phi2);
      yy = r * cos (phi2);

      if (phi >= 1.5 * G_PI)
        {
          x_calc = (double)xm - xx;
          y_calc = (double)ym - yy;
        }
      else if (phi >= G_PI)
        {
          x_calc = (double)xm - xx;
          y_calc = (double)ym + yy;
        }
      else if (phi >= 0.5 * G_PI)
        {
          x_calc = (double)xm + xx;
          y_calc = (double)ym + yy;
        }
      else
        {
          x_calc = (double)xm + xx;
          y_calc = (double)ym - yy;
        }
    }

  xi = (int) (x_calc + 0.5);
  yi = (int) (y_calc + 0.5);

  inside = (WITHIN (0, xi, boundary.width - 1) && WITHIN (0, yi, boundary.height - 1));
  if (inside)
    {
      *x = x_calc;
      *y = y_calc;
    }
  return inside;
}


static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy(&result, in_rect);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o            = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle            boundary     = get_effective_area (operation);
  const Babl              *format       = babl_format ("RGBA float");

  gint      x,y;
  gfloat   *src_buf, *dst_buf;
  gfloat    dest[4];
  gint      i, offset = 0;
  gboolean  inside;
  gdouble   px, py;

  GeglMatrix2  scale;        /* a matrix indicating scaling factors around the
                                current center pixel.
                             */

  src_buf = g_new0 (gfloat, result->width * result->height * 4);
  dst_buf = g_new0 (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, result, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (o->middle)
    {
      o->pole_x = boundary.width / 2;
      o->pole_y = boundary.height / 2;
    }

  for (y = result->y; y < result->y + result->height; y++)
    for (x = result->x; x < result->x + result->width; x++)
      {
#define gegl_unmap(u,v,ud,vd) {                                         \
          gdouble rx, ry;                                               \
          inside = calc_undistorted_coords ((gdouble)x, (gdouble)y,     \
                                            &rx, &ry, o, boundary);     \
          ud = rx;                                                      \
          vd = ry;                                                      \
        }
        gegl_sampler_compute_scale (scale, x, y);
        gegl_unmap(x,y,px,py);
#undef gegl_unmap

        if (inside)
          gegl_buffer_sample (input, px, py, &scale, dest, format,
                              GEGL_SAMPLER_LOHALO, GEGL_ABYSS_NONE);
        else
          for (i=0; i<4; i++)
            dest[i] = 0.0;

        for (i=0; i<4; i++)
          dst_buf[offset++] = dest[i];
      }

  gegl_buffer_set (output, result, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  return  TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:polar-coordinates",
    "categories" , "enhance",
    "description", _("Performs polar-coordinates on the image."),
    NULL);
}

#endif
