/* This file is an image processing operation for GEGL.
 *
 * This GEGL operation is a port of the main part of the Fractal
 * Explorer plug-in from GIMP. Fractal Explorer (Version 2) was
 * originally written by Daniel Cotting (cotting@multimania.com).
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
 * Copyright 2006 Kevin Cozens <kcozens@cvs.gnome.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define MAXNCOLORS 8192

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (width,  _("Width"),  10, 10000000, 400, _("Width"))
gegl_chant_int (height, _("Height"), 10, 10000000, 400, _("Height"))

gegl_chant_int (fractaltype, _("Fractal type"), 0, 8, 0, _("Fractal Type"))

gegl_chant_double (xmin, _("Left"),   -3.0, 3.0, -2.0, _("Left"))
gegl_chant_double (xmax, _("Right"),  -3.0, 3.0,  2.0, _("Right"))
gegl_chant_double (ymin, _("Top"),    -3.0, 3.0, -2.0, _("Top"))
gegl_chant_double (ymax, _("Bottom"), -3.0, 3.0,  2.0, _("Bottom"))

gegl_chant_int (iter, _("Iterations"), 1, 1000, 50, _("Iterations"))

gegl_chant_double (cx, _("CX"), -2.5, 2.5, -0.75, _("CX (only Julia)"))
gegl_chant_double (cy, _("CY"), -2.5, 2.5,  0.2,  _("CY (only Julia)"))

gegl_chant_double (redstretch,   _("Red stretch"),   0.0, 1.0, 1.0,
                   _("Red stretching factor"))
gegl_chant_double (greenstretch, _("Green stretch"), 0.0, 1.0, 1.0,
                   _("Green stretching factor"))
gegl_chant_double (bluestretch,  _("Blue stretch"),  0.0, 1.0, 1.0,
                   _("Blue stretching factor"))

gegl_chant_int (redmode,   _("Red mode"),   0, 2, 1,
                _("Red application mode (0:SIN; 1:COS; 2:NONE)"))
gegl_chant_int (greenmode, _("Green mode"), 0, 2, 1,
                _("Green application mode (0:SIN; 1:COS; 2:NONE)"))
gegl_chant_int (bluemode,  _("Blue mode"),  0, 2, 0,
                _("Blue application mode (0:SIN; 1:COS; 2:NONE)"))

gegl_chant_boolean (redinvert,   _("Red inversion"),   FALSE,
                    _("Red inversion"))
gegl_chant_boolean (greeninvert, _("Green inversion"), FALSE,
                    _("Green inversion"))
gegl_chant_boolean (blueinvert,  _("Blue inversion"),  FALSE,
                    _("Blue inversion"))

gegl_chant_int (ncolors, _("Colors"), 2, MAXNCOLORS, 256,
                _("Number of colors"))

gegl_chant_boolean (useloglog, _("Loglog smoothing"), FALSE,
                    _("Use loglog smoothing"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "fractal-explorer.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

enum
{
  SINUS,
  COSINUS,
  NONE
};

enum
{
  TYPE_MANDELBROT,
  TYPE_JULIA,
  TYPE_BARNSLEY_1,
  TYPE_BARNSLEY_2,
  TYPE_BARNSLEY_3,
  TYPE_SPIDER,
  TYPE_MAN_O_WAR,
  TYPE_LAMBDA,
  TYPE_SIERPINSKI,
  NUM_TYPES
};

typedef struct
{
  guchar r, g, b;
} gucharRGB;

typedef gucharRGB  clrmap[MAXNCOLORS];


static void
explorer_render_row (GeglChantO *o,
                     gint        col_start,
                     gint        col_end,
                     gint        row,
                     clrmap      colormap,
                     guchar    **dest_row)
{
  gint    fractaltype;
  gint    col;
  gdouble xmin;
  gdouble ymin;
  gdouble a;
  gdouble b;
  gdouble x;
  gdouble y;
  gdouble oldx;
  gdouble oldy;
  gdouble tempsqrx;
  gdouble tempsqry;
  gdouble tmpx = 0;
  gdouble tmpy = 0;
  gdouble foldxinitx;
  gdouble foldxinity;
  gdouble foldyinitx;
  gdouble foldyinity;
  gdouble xx = 0;
  gdouble adjust;
  gdouble cx;
  gdouble cy;
  gint    counter;
  gint    color;
  gint    iteration;
  gint    ncolors;
  gint    useloglog;
  gdouble xdiff;
  gdouble ydiff;
  gdouble log2;

  fractaltype = o->fractaltype;
  xmin = o->xmin;
  ymin = o->ymin;
  cx = o->cx;
  cy = o->cy;
  iteration = o->iter;
  ncolors = o->ncolors;
  useloglog = o->useloglog;
  log2 = log (2.0);

  xdiff = (o->xmax - xmin) / o->width;
  ydiff = (o->ymax - ymin) / o->height;

  for (col = col_start; col < col_end; col++)
    {
      a = xmin + (gdouble) col * xdiff;
      b = ymin + (gdouble) row * ydiff;
      if (fractaltype != 0)
        {
          tmpx = x = a;
          tmpy = y = b;
        }
      else
        {
          x = 0;
          y = 0;
        }

      for (counter = 0; counter < iteration; counter++)
        {
          oldx=x;
          oldy=y;

          switch (fractaltype)
            {
            case TYPE_MANDELBROT:
              xx = x * x - y * y + a;
              y = 2.0 * x * y + b;
              break;

            case TYPE_JULIA:
              xx = x * x - y * y + cx;
              y = 2.0 * x * y + cy;
              break;

            case TYPE_BARNSLEY_1:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (oldx >= 0)
                {
                  xx = (foldxinitx - cx - foldyinity);
                  y  = (foldyinitx - cy + foldxinity);
                }
              else
                {
                  xx = (foldxinitx + cx - foldyinity);
                  y  = (foldyinitx + cy + foldxinity);
                }
              break;

            case TYPE_BARNSLEY_2:
              foldxinitx = oldx * cx;
              foldyinity = oldy * cy;
              foldxinity = oldx * cy;
              foldyinitx = oldy * cx;
              /* orbit calculation */
              if (foldxinity + foldyinitx >= 0)
                {
                  xx = foldxinitx - cx - foldyinity;
                  y  = foldyinitx - cy + foldxinity;
                }
              else
                {
                  xx = foldxinitx + cx - foldyinity;
                  y  = foldyinitx + cy + foldxinity;
                }
              break;

            case TYPE_BARNSLEY_3:
              foldxinitx  = oldx * oldx;
              foldyinity  = oldy * oldy;
              foldxinity  = oldx * oldy;
              /* orbit calculation */
              if (oldx > 0)
                {
                  xx = foldxinitx - foldyinity - 1.0;
                  y  = foldxinity * 2;
                }
              else
                {
                  xx = foldxinitx - foldyinity -1.0 + cx * oldx;
                  y  = foldxinity * 2;
                  y += cy * oldx;
                }
              break;

            case TYPE_SPIDER:
              /* { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
              xx = x*x - y*y + tmpx + cx;
              y = 2 * oldx * oldy + tmpy +cy;
              tmpx = tmpx/2 + xx;
              tmpy = tmpy/2 + y;
              break;

            case TYPE_MAN_O_WAR:
              xx = x*x - y*y + tmpx + cx;
              y = 2.0 * x * y + tmpy + cy;
              tmpx = oldx;
              tmpy = oldy;
              break;

            case TYPE_LAMBDA:
              tempsqrx = x * x;
              tempsqry = y * y;
              tempsqrx = oldx - tempsqrx + tempsqry;
              tempsqry = -(oldy * oldx);
              tempsqry += tempsqry + oldy;
              xx = cx * tempsqrx - cy * tempsqry;
              y = cx * tempsqry + cy * tempsqrx;
              break;

            case TYPE_SIERPINSKI:
              xx = oldx + oldx;
              y = oldy + oldy;
              if (oldy > .5)
                y = y - 1;
              else if (oldx > .5)
                xx = xx - 1;
              break;

            default:
              break;
            }

          x = xx;

          if (((x * x) + (y * y)) >= 4.0)
            break;
        }

      if (useloglog)
        {
          gdouble modulus_square = (x * x) + (y * y);

          if (modulus_square > (G_E * G_E))
              adjust = log (log (modulus_square) / 2.0) / log2;
          else
              adjust = 0.0;
        }
      else
        {
          adjust = 0.0;
        }

      color = (gint) (((counter - adjust) * (ncolors - 1)) / iteration);

      (*dest_row)[0] = colormap[color].r;
      (*dest_row)[1] = colormap[color].g;
      (*dest_row)[2] = colormap[color].b;
      (*dest_row) += 3;
    }
}

static void
make_color_map (GeglChantO *o, clrmap colormap)
{
  gint     i;
  gint     r;
  gint     gr;
  gint     bl;
  gdouble  redstretch;
  gdouble  greenstretch;
  gdouble  bluestretch;
  gdouble  pi = atan (1) * 4;

  redstretch   = o->redstretch * 127.5;
  greenstretch = o->greenstretch * 127.5;
  bluestretch  = o->bluestretch * 127.5;

  for (i = 0; i < o->ncolors; i++)
    {
      double x = (i*2.0) / o->ncolors;
      r = gr = bl = 0;

      switch (o->redmode)
        {
        case SINUS:
          r = (int) redstretch *(1.0 + sin((x - 1) * pi));
          break;
        case COSINUS:
          r = (int) redstretch *(1.0 + cos((x - 1) * pi));
          break;
        case NONE:
          r = (int)(redstretch *(x));
          break;
        default:
          break;
        }

      switch (o->greenmode)
        {
        case SINUS:
          gr = (int) greenstretch *(1.0 + sin((x - 1) * pi));
          break;
        case COSINUS:
          gr = (int) greenstretch *(1.0 + cos((x - 1) * pi));
          break;
        case NONE:
          gr = (int)(greenstretch *(x));
          break;
        default:
          break;
        }

      switch (o->bluemode)
        {
        case SINUS:
          bl = (int) bluestretch * (1.0 + sin ((x - 1) * pi));
          break;
        case COSINUS:
          bl = (int) bluestretch * (1.0 + cos ((x - 1) * pi));
          break;
        case NONE:
          bl = (int) (bluestretch * x);
          break;
        default:
          break;
        }

      r  = MIN (r,  255);
      gr = MIN (gr, 255);
      bl = MIN (bl, 255);

      if (o->redinvert)
        r = 255 - r;

      if (o->greeninvert)
        gr = 255 - gr;

      if (o->blueinvert)
        bl = 255 - bl;

      colormap[i].r = r;
      colormap[i].g = gr;
      colormap[i].b = bl;
    }
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u8"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  result = {0,0,0,0};

  result.width  = o->width;
  result.height = o->height;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  clrmap  colormap;
  guchar *buf;
  gint    pxsize;

  make_color_map (o, colormap);

  g_object_get (output, "px-size", &pxsize, NULL);

  buf  = g_new (guchar, result->width * result->height * pxsize);
    {
      guchar *dst=buf;
      gint y;
      for (y=0; y < result->height; y++)
        {
          explorer_render_row (o,
                               result->x,
                               result->x + result->width ,
                               result->y + y,
                               colormap,
                               &dst);
        }
    }

  gegl_buffer_set (output, NULL, babl_format ("R'G'B' u8"), buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (buf);

  return TRUE;
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

  operation_class->name        = "gegl:fractal-explorer";
  operation_class->categories  = "render";
  operation_class->description = _("Fractal Explorer");

  operation_class->no_cache = TRUE;
  operation_class->get_cached_region = NULL;
}

#endif
