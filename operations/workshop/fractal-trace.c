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
 * Copyright (C) 1997 Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (fractal, _("Fractal"), "fractal",
                   _("Type of fractal to use. "
                     "Choices are julia, mandelbrot. Default is mandelbrot."))
gegl_chant_double (X1, _("X1"), -50.0, 50.0, -1.00,
                  _("X1 value, position"))
gegl_chant_double (X2, _("X2"), -50.0, 50.0, 0.50,
                  _("X2 value, position"))
gegl_chant_double (Y1, _("Y1"), -50.0, 50.0, -1.00,
                  _("X2 value, position"))
gegl_chant_double (Y2, _("Y2"), -50.0, 50.0, 1.00,
                  _("Y2 value, position"))
gegl_chant_double (JX, _("JX"), -50.0, 50.0, 0.5,
                   _("Julia seed X value, position"))
gegl_chant_double (JY, _("JY"), -50.0, 50.0, 0.5,
                   _("Julia seed Y value, position"))
gegl_chant_int    (depth, _("Depth"), 1, 50, 3,
                  _("Depth value"))
gegl_chant_string (background, _("Background"), "wrap",
                  _("Optional parameter to override automatic selection of wrap background. "
                    "Choices are wrap, black, white and transparent"))

# else

#define GEGL_CHANT_TYPE_FILTER

#define GEGL_CHANT_C_FILE       "fractal-trace.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

enum OutsideType
{
  OUTSIDE_TYPE_WRAP,
  OUTSIDE_TYPE_TRANSPARENT,
  OUTSIDE_TYPE_BLACK,
  OUTSIDE_TYPE_WHITE 
};

enum FractalType
{
  FRACTAL_TYPE_MANDEL,
  FRACTAL_TYPE_JULIA
};


static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}   

static void
julia (gdouble  x,
       gdouble  y,
       gdouble  jx,
       gdouble  jy,
       gdouble *u,
       gdouble *v,
       gint     depth,
       gdouble  escape_radius)
{
  gint    i;
  gdouble xx = x;
  gdouble yy = y;

  for (i = 0; i < depth; i++)
    {
      gdouble x2, y2, tmp;

      x2 = xx * xx;
      y2 = yy * yy;
      tmp = x2 - y2 + jx;
      yy  = 2 * xx * yy + jy;
      xx  = tmp;
      /*commented this line because it is bugy */
      /*if (x2 + y2 > (pow(escape_radius,2))) break;*/
    }

  *u = xx;
  *v = yy;
}


static void
fractaltrace (GeglBuffer          *input,
              const GeglRectangle *picture,
              gfloat              *dst_buf,
              const GeglRectangle *roi,
              GeglChantO          *o,
              gint                 y,
              enum FractalType     fractal,
              enum OutsideType     id,
              Babl                *format
              )
{
  gint           x, i, offset;
  gdouble        scale_x, scale_y;
  gdouble        cx, cy;
  gdouble        px, py;
  gfloat         dest[4];
  gdouble        escape_radius = sqrt(pow(o->X2 - o->X1,2) + pow(o->Y2 - o->Y1,2));

  scale_x = (gdouble) (o->X2 - o->X1) / (gdouble) picture->width;
  scale_y = (gdouble) (o->Y2 - o->Y1) / (gdouble) picture->height;

  cy = (gdouble) o->Y1 + ((gdouble) (y - picture->y)) * scale_y;
  offset = (y - roi->y) * roi->width * 4;

  for (x = roi->x; x < roi->x + roi->width; x++)
    {
       dest[1] = dest[2] = dest[3] = dest[0] = 0.0;
       cx = o->X1 + (x - picture->x) * scale_x;

       switch (fractal)
	{
        case FRACTAL_TYPE_JULIA:
          julia (cx, cy, o->JX, o->JY, &px, &py, o->depth, escape_radius);
          break;
        case FRACTAL_TYPE_MANDEL:
          julia (cx, cy, cx, cy, &px, &py, o->depth, escape_radius);
          break;
        default:
          g_error (_("Unsupported fractal type"));
        }

       px = (px - o->X1) / scale_x + picture->x;
       py = (py - o->Y1) / scale_y + picture->y;

       if (0 <= px && px < picture->width && 0 <= py && py < picture->height)
          {
          gegl_buffer_sample (input, px, py, 1.0, dest, format,
                             GEGL_INTERPOLATION_LINEAR);
          }
       else
         {
           switch (id)
             {
             case OUTSIDE_TYPE_WRAP:
               px = fmod (px, picture->width);
               py = fmod (py, picture->height);
               if (px < 0.0) px += picture->width;
               if (py < 0.0) py += picture->height;
               /*associating to NaN and -NaN values, if there is the case*/

               if (isnan(px))
                  {
                  if (signbit(px)) 
                      {
                      px = 0.0;
                      }
                  else 
                      {
                      px = (gdouble) picture->width - 1.0;
                      }
                  }

               if (isnan(py))
                  {
                  if (signbit(py)) 
                       {
                       py = 0.0;
                       }
                  else  
                       {
                       py = (gdouble) picture->height -1.0;
                       }
                  }

               gegl_buffer_sample (input, px, py, 1.0, dest, format, 
                                  GEGL_INTERPOLATION_LINEAR);
               break;
             case OUTSIDE_TYPE_TRANSPARENT:
               dest[0] = dest[1] = dest[2] = dest[3] = 0.0;
               break;
             case OUTSIDE_TYPE_BLACK:
               dest[0] = dest[1] = dest[2] = 0.0;
               dest[3] = 1.0;
               break;
             case OUTSIDE_TYPE_WHITE:
               dest[0] = dest[1] = dest[2] = dest[3] = 1.0;
               break;
             }
         }
      /*upload pixel value is destination buffer*/
        for(i = 0;i < 4; i++)
	  dst_buf[offset++] = dest[i];
    }
}


static gboolean 
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO    *o            = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  boundary     = gegl_operation_get_bounding_box (operation); 
  Babl          *format       = babl_format ("RGBA float");

  enum FractalType  frT;
  enum OutsideType  id;
  gfloat           *dst_buf;
  gint              y;

  frT = FRACTAL_TYPE_MANDEL;
  if (!strcmp (o->fractal, "mandel"))
    frT = FRACTAL_TYPE_MANDEL;
  else if (!strcmp(o->fractal, "julia"))
    frT = FRACTAL_TYPE_JULIA;


  id = OUTSIDE_TYPE_WRAP; /*wrap is default*/
  if (!strcmp(o->background,"wrap"))
      id = OUTSIDE_TYPE_WRAP;
  else if (!strcmp(o->background,"transparent")) 
      id = OUTSIDE_TYPE_TRANSPARENT;
  else if (!strcmp(o->background,"black"))       
      id = OUTSIDE_TYPE_BLACK;
  else if (!strcmp(o->background,"white"))
      id = OUTSIDE_TYPE_WHITE;

  dst_buf = g_new0 (gfloat, result->width * result->height * 4);

  for (y = result->y; y < result->y + result->height; y++)
	fractaltrace(input, &boundary, dst_buf, result, o, y, frT, id, format);

  gegl_buffer_set (output, result, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);

  return TRUE;
}

static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy(&result, in_rect);

  return result;
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (!in_rect){
        return result;
  }

  gegl_rectangle_copy(&result, in_rect);

  #ifdef TRACE
    g_warning ("< get_bounding_box result = %dx%d+%d+%d", result.width, result.height, result.x, result.y);
  #endif
  return result;
}


/* Compute the input rectangle required to compute the specified region of interest (roi).
 */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle  result = get_effective_area (operation);

  #ifdef TRACE
    g_warning ("> get_required_for_output src=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
    if (roi)
      g_warning ("  ROI == %dx%d+%d+%d", roi->width, roi->height, roi->x, roi->y);
  #endif

  return result;
}


static void 
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  operation_class->categories = "map";
  operation_class->name       = "gegl:fractal-trace";
  operation_class->description =
          _("Performs fractal trace on the image");
}

#endif
