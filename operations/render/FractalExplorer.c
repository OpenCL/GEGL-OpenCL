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

#define MAXNCOLORS 8192

#if GEGL_CHANT_PROPERTIES

gegl_chant_int (width,  10, 10000000, 400, "Width")
gegl_chant_int (height, 10, 10000000, 400, "Height")

gegl_chant_int (fractaltype, 0, 8, 0, "Fractal Type")
gegl_chant_double (xmin, -3.0, 3.0, -2.0, "Left")
gegl_chant_double (xmax, -3.0, 3.0, 2.0, "Right")
gegl_chant_double (ymin, -3.0, 3.0, -2.0, "Top")
gegl_chant_double (ymax, -3.0, 3.0, 2.0, "Bottom")
gegl_chant_int (iter, 1, 1000, 50, "Iterations")
gegl_chant_double (cx, -2.5, 2.5, -0.75, "CX (Only Julia)")
gegl_chant_double (cy, -2.5, 2.5,  0.2,  "CY (Only Julia)")

gegl_chant_double (redstretch,   0.0, 1.0, 1.0, "Red stretching factor")
gegl_chant_double (greenstretch, 0.0, 1.0, 1.0, "Green stretching factor")
gegl_chant_double (bluestretch,  0.0, 1.0, 1.0, "Blue stretching factor")
gegl_chant_int (redmode,   0, 2, 1, "Red application mode (0:SIN; 1:COS; 2:NONE)")
gegl_chant_int (greenmode, 0, 2, 1, "Green application mode (0:SIN; 1:COS; 2:NONE)")
gegl_chant_int (bluemode,  0, 2, 0, "Blue application mode (0:SIN; 1:COS; 2:NONE)")
gegl_chant_boolean (redinvert,   FALSE, "Red inversion")
gegl_chant_boolean (greeninvert, FALSE, "Green inversion")
gegl_chant_boolean (blueinvert,  FALSE, "Blue inversion")

gegl_chant_int (ncolors, 2, MAXNCOLORS, 256, "Number of colors")

gegl_chant_boolean (useloglog,  FALSE, "Use loglog smoothing")

#else

#define GEGL_CHANT_NAME           FractalExplorer
#define GEGL_CHANT_SELF           "FractalExplorer.c"
#define GEGL_CHANT_DESCRIPTION    "Fractal Explorer"
#define GEGL_CHANT_CATEGORIES     "render"

#define GEGL_CHANT_SOURCE

#define GEGL_CHANT_PREPARE
#define GEGL_CHANT_CLASS_INIT

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
explorer_render_row (GeglChantOperation *self,
                     gint                col_start,
                     gint                col_end,
                     gint                row,
                     clrmap              colormap,
                     guchar            **dest_row)
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

  fractaltype = self->fractaltype;
  xmin = self->xmin;
  ymin = self->ymin;
  cx = self->cx;
  cy = self->cy;
  iteration = self->iter;
  ncolors = self->ncolors;
  useloglog = self->useloglog;
  log2 = log (2.0);

  xdiff = (self->xmax - xmin) / self->width;
  ydiff = (self->ymax - ymin) / self->height;

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
make_color_map (GeglChantOperation *self, clrmap colormap)
{
  gint     i;
  gint     r;
  gint     gr;
  gint     bl;
  gdouble  redstretch;
  gdouble  greenstretch;
  gdouble  bluestretch;
  gdouble  pi = atan (1) * 4;

  redstretch   = self->redstretch * 127.5;
  greenstretch = self->greenstretch * 127.5;
  bluestretch  = self->bluestretch * 127.5;

  for (i = 0; i < self->ncolors; i++)
    {
      double x = (i*2.0) / self->ncolors;
      r = gr = bl = 0;

      switch (self->redmode)
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

      switch (self->greenmode)
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

      switch (self->bluemode)
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

      if (self->redinvert)
        r = 255 - r;

      if (self->greeninvert)
        gr = 255 - gr;

      if (self->blueinvert)
        bl = 255 - bl;

      colormap[i].r = r;
      colormap[i].g = gr;
      colormap[i].b = bl;
    }
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  const GeglRectangle *need;
  GeglBuffer          *output = NULL;

  need = gegl_operation_get_requested_region (operation, context_id);
  {
    const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
    clrmap               colormap;
    guchar              *buf;

    make_color_map (self, colormap);

    output = gegl_operation_get_target (operation, context_id, "output");

    buf  = g_new (guchar, result->width * result->height * 4);
      {
        guchar *dst=buf;
        gint y;
        for (y=0; y < result->height; y++)
          {
            explorer_render_row (self,
                                 result->x,
                                 result->x + result->width ,
                                 result->y + y,
                                 colormap,
                                 &dst);
          }
      }

    gegl_buffer_set (output, NULL, babl_format ("R'G'B' u8"), buf);
    g_free (buf);
  }
  return TRUE;
}

static void
prepare (GeglOperation *operation,
         gpointer       context_id)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u8"));
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglRectangle       result = {0,0,0,0};

  result.width  = self->width;
  result.height  = self->height;

  return result;
}

static void class_init (GeglOperationClass *klass)
{
  klass->adjust_result_region = NULL;
  klass->no_cache = FALSE;
}

#endif
