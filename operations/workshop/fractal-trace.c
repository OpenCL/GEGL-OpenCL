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
gegl_chant_int    (depth, _("Depth"), 1, 65536, 3,
                   _("Depth value"))
gegl_chant_double (bailout, _("Bailout"), 0.0, G_MAXDOUBLE, G_MAXDOUBLE,
                   _("Bailout length"))
gegl_chant_string (background, _("Background"), "wrap",
                   _("Optional parameter to override automatic selection of wrap background. "
                     "Choices are wrap, black, white and transparent."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "fractal-trace.c"

#include "gegl-chant.h"
#include <math.h>

typedef enum
{
  BACKGROUND_TYPE_WRAP,
  BACKGROUND_TYPE_TRANSPARENT,
  BACKGROUND_TYPE_BLACK,
  BACKGROUND_TYPE_WHITE
} BackgroundType;

typedef enum
{
  FRACTAL_TYPE_MANDELBROT,
  FRACTAL_TYPE_JULIA
} FractalType;

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
       gdouble  bailout2)
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

      if ((x2 + y2) > bailout2)
	break;
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
              FractalType          fractal_type,
              BackgroundType       background_type,
              const Babl          *format)
{
  GeglMatrix2  scale;        /* a matrix indicating scaling factors around the
                                current center pixel.
                              */
  gint         x, i, offset;
  gdouble      scale_x, scale_y;
  gdouble      bailout2;
  gfloat       dest[4];

  scale_x = (o->X2 - o->X1) / picture->width;
  scale_y = (o->Y2 - o->Y1) / picture->height;

  bailout2 = o->bailout * o->bailout;

  offset = (y - roi->y) * roi->width * 4;

  for (x = roi->x; x < roi->x + roi->width; x++)
    {
      gdouble cx, cy;
      gdouble px, py;
      dest[1] = dest[2] = dest[3] = dest[0] = 0.0;

      switch (fractal_type)
        {
        case FRACTAL_TYPE_JULIA:
#define gegl_unmap(u,v,ud,vd) {\
       gdouble rx, ry;\
       cx = o->X1 + ((u) - picture->x) * scale_x; \
       cy = o->Y1 + ((v) - picture->y) * scale_y; \
       julia (cx, cy, o->JX, o->JY, &rx, &ry, o->depth, bailout2);\
       ud = (rx - o->X1) / scale_x + picture->x;\
       vd = (ry - o->Y1) / scale_y + picture->y;\
      }
      gegl_sampler_compute_scale (scale, x, y);
      gegl_unmap(x,y,px,py);
#undef gegl_unmap
          break;

        case FRACTAL_TYPE_MANDELBROT:
#define gegl_unmap(u,v,ud,vd) {\
           gdouble rx, ry;\
           cx = o->X1 + ((u) - picture->x) * scale_x; \
           cy = o->Y1 + ((v) - picture->y) * scale_y; \
           julia (cx, cy, cx, cy, &rx, &ry, o->depth, bailout2);\
           ud = (rx - o->X1) / scale_x + picture->x;\
           vd = (ry - o->Y1) / scale_y + picture->y;\
         }
      gegl_sampler_compute_scale (scale, x, y);
      gegl_unmap(x,y,px,py);
#undef gegl_unmap
          break;

        default:
          g_error (_("Unsupported fractal type"));
        }

      if (0 <= px && px < picture->width && 0 <= py && py < picture->height)
        {
          gegl_buffer_sample (input, px, py, &scale, dest, format,
                              GEGL_SAMPLER_LOHALO, GEGL_ABYSS_NONE);
        }
      else
        {
          switch (background_type)
            {
            case BACKGROUND_TYPE_WRAP:
              px = fmod (px, picture->width);
              py = fmod (py, picture->height);

              if (px < 0.0)
                px += picture->width;
              if (py < 0.0)
                py += picture->height;

              /* Check for NaN */
              if (isnan (px))
                {
                  if (signbit (px))
                    px = 0.0;
                  else
                    px = picture->width - 1.0;
                }

              if (isnan (py))
                {
                  if (signbit (py))
                    py = 0.0;
                  else
                    py = picture->height - 1.0;
                }

              gegl_buffer_sample (input, px, py, &scale, dest, format,
                                  GEGL_SAMPLER_LOHALO, GEGL_ABYSS_NONE);
              break;

            case BACKGROUND_TYPE_TRANSPARENT:
              dest[0] = dest[1] = dest[2] = dest[3] = 0.0;
              break;

            case BACKGROUND_TYPE_BLACK:
              dest[0] = dest[1] = dest[2] = 0.0;
              dest[3] = 1.0;
              break;

            case BACKGROUND_TYPE_WHITE:
              dest[0] = dest[1] = dest[2] = dest[3] = 1.0;
              break;
            }
        }

      for (i = 0; i < 4; i++)
        dst_buf[offset++] = dest[i];
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO	*o;
  GeglRectangle  boundary;
  const Babl    *format;
  FractalType    fractal_type;
  BackgroundType background_type;
  gfloat        *dst_buf;
  gint           y;

  o = GEGL_CHANT_PROPERTIES (operation);
  boundary = gegl_operation_get_bounding_box (operation);

  fractal_type = FRACTAL_TYPE_MANDELBROT;
  if (!strcmp (o->fractal, "mandelbrot"))
    fractal_type = FRACTAL_TYPE_MANDELBROT;
  else if (!strcmp(o->fractal, "julia"))
    fractal_type = FRACTAL_TYPE_JULIA;

  background_type = BACKGROUND_TYPE_WRAP;
  if (!strcmp (o->background, "wrap"))
    background_type = BACKGROUND_TYPE_WRAP;
  else if (!strcmp (o->background, "transparent"))
    background_type = BACKGROUND_TYPE_TRANSPARENT;
  else if (!strcmp (o->background, "black"))
    background_type = BACKGROUND_TYPE_BLACK;
  else if (!strcmp (o->background, "white"))
    background_type = BACKGROUND_TYPE_WHITE;

  format = babl_format ("RGBA float");
  dst_buf = g_new0 (gfloat, result->width * result->height * 4);

  for (y = result->y; y < result->y + result->height; y++)
    fractaltrace (input, &boundary, dst_buf, result, o, y,
                  fractal_type, background_type, format);

  gegl_buffer_set (output, result, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);

  gegl_buffer_sample_cleanup (input);

  return TRUE;
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

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "categories"  , "map",
    "name"        , "gegl:fractal-trace",
    "description" , _("Performs fractal trace on the image"),
    NULL);
}

#endif
