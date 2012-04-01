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
 * TMO:
 * Copyright 2010      Danny Robson      <danny@blubinc.net>
 * (pfstmo)  2003-2004 Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
 *
 * PDE:
 * 2003-2004 Grzegorz Krawczyk  <krawczyk@mpi-sb.mpg.de>
 *           Rafal Mantiuk      <mantiuk@mpi-sb.mpg.de>
 * Some code from Numerical Recipes in C
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (alpha, _("Alpha"),
                  0.0, 2.0, 1.0,
                  _("Gradient threshold for detail enhancement"))
gegl_chant_double (beta, _("Beta"),
                  0.1, 2.0, 0.9,
                  _("Strength of local detail enhancement"))
gegl_chant_double (saturation, _("Saturation"),
                  0.0, 1.0, 0.8,
                  _("Global color saturation factor"))
gegl_chant_double (noise, _("Noise"),
                  0.0, 1.0, 0.0,
                  _("Gradient threshold for lowering detail enhancement"))


#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "fattal02.c"

#include "gegl-chant.h"
#include "gegl-debug.h"
#include <stdlib.h>

static const gchar *OUTPUT_FORMAT   = "RGB float";
static const gint   MINIMUM_PYRAMID = 32;

/* I: pixel buffer, luminance with stride of 1
 * R: rectangle, describes the buffer extent
 * X: width coordinate
 * Y: height coordinate
 */
#define _P(I,R,X,Y) ((I)[(Y) * (R)->width + (X)])

/* The width/height of the pyramid at a level */
#define LEVEL_WIDTH(extent, level)  ((extent)->width  / (1 << (level)))
#define LEVEL_HEIGHT(extent, level) ((extent)->height / (1 << (level)))
#define LEVEL_EXTENT(extent, level)       \
    ((GeglRectangle){                     \
        0, 0,                             \
        LEVEL_WIDTH  ((extent), (level)), \
        LEVEL_HEIGHT ((extent), (level))  \
    })
#define LEVEL_SIZE(extent, level) (LEVEL_EXTENT((extent), (level)).width * \
                                   LEVEL_EXTENT((extent), (level)).height)

#define MODYF 0 /* 1 or 0 (1 is better) */
#define MINS 16	/* minimum size 4 6 or 100 */

/* #define MODYF_SQRT -1.0f *//* -1 or 0 */
#define SMOOTH_IT 1 /* minimum 1  */
#define BCG_STEPS 20
#define V_CYCLE 2 /* number of v-cycles  */

/* precision */
#define EPS 1.0e-12

static void
linbcg (guint   rows,
        guint   cols,
        gfloat  b[],
        gfloat  x[],
        gint    itol,
        gfloat  tol,
        gint    itmax,
        gint   *iter,
        gfloat *err);


/**
 * Set all elements of the array to a give value.
 *
 * @param array array to modify
 * @param value all elements of the array will be set to this value
 */
static inline void
fattal02_set_array (gfloat *array,
                    guint   size,
                    gfloat  value)
{
  guint i;
  for (i = 0; i < size; ++i)
    array[i] = value;
}


static inline void
fattal02_add_array (gfloat       *accum,
                    guint         size,
                    const gfloat *input)
{
  guint i;
  for (i = 0; i < size; ++i)
    accum[i] += input[i];
}


static inline void
fattal02_copy_array (const gfloat *input,
                     gsize         size,
                     gfloat       *output)
{
  memcpy (output, input, size * sizeof (input[0]));
}


/*
 * Full Multigrid Algorithm for solving partial differential equations
 */

static void
fattal02_restrict (const gfloat        *input,
                   const GeglRectangle *extent_i,
                   gfloat              *output,
                   const GeglRectangle *extent_o)
{
  const guint inRows = extent_i->height,
              inCols = extent_i->width;

  const guint outRows = extent_o->height,
              outCols = extent_o->width;

  const gfloat dx = (gfloat)inCols / (gfloat)outCols,
               dy = (gfloat)inRows / (gfloat)outRows;

  const gfloat filterSize = 0.5;

  gfloat sx, sy;
  guint   x,  y;

  for (y = 0, sy = dy / 2 - 0.5; y < outRows; ++y, sy += dy)
    {
      for (x = 0, sx = dx / 2 - 0.5; x < outCols; ++x, sx += dx )
        {
          gfloat pixVal = 0;
          gfloat w      = 0;
          gint   ix, iy;

          for (ix  = MAX (0, ceilf (sx - dx * filterSize));
               ix <= MIN (floorf (sx + dx * filterSize), inCols - 1);
               ++ix)
            {
              for (iy  = MAX (0, ceilf (sy - dx * filterSize));
                   iy <= MIN (floorf (sy + dx * filterSize), inRows - 1);
                   ++iy)
                {
                  pixVal += input[ix + iy * inCols];
                  w      += 1;
                }
            }

          output[x + y * outCols] = pixVal / w;
        }
    }
}


static void
fattal02_prolongate (const gfloat        *input,
                     const GeglRectangle *extent_i,
                     gfloat              *output,
                     const GeglRectangle *extent_o)
{
  gfloat dx = (gfloat)extent_i->width  / (gfloat)extent_o->width,
         dy = (gfloat)extent_i->height / (gfloat)extent_o->height;

  const guint outRows = extent_o->height,
              outCols = extent_o->width;

  const gfloat inRows = extent_i->height,
               inCols = extent_i->width;

  const float filterSize = 1;

  gfloat sx, sy;
  guint   x,  y;

  for (y = 0, sy = -dy / 2; y < outRows; ++y, sy += dy)
    {
      for (x = 0, sx = -dx / 2; x < outCols; ++x, sx += dx )
        {
          gfloat pixVal = 0;
          gfloat weight = 0;
          gfloat ix, iy;

          for (ix  = MAX (0, ceilf (sx - filterSize));
               ix <= MIN (floorf (sx + filterSize), inCols - 1);
               ++ix)
            {
              for (iy  = MAX (0, ceilf (sy - filterSize));
                   iy <= MIN (floorf (sy + filterSize), inRows - 1);
                   ++iy)
                {
                  const gfloat fx   = fabs (sx - ix),
                               fy   = fabs (sy - iy),
                               fval = (1 - fx) * (1 - fy);

                  pixVal += input[(guint)ix + (guint)iy * (guint)inCols] * fval;
                  weight += fval;
                }
            }

          g_return_if_fail (weight != 0);

          output [x + y * outCols] = pixVal / weight;
        }
    }
}


static void
fattal02_exact_solution (gfloat              *F,
                         const GeglRectangle *extent_f,
                         gfloat              *U,
                         const GeglRectangle *extent_u)
{
  /* pfstmo suggests that successive over-relaxation should be used here,
   * followed by scaling by the square of the inverse of the sqrt of the array
   * length. However it was commented out due to 'incorrect results', and the
   * array zeroing was used in its place.
   */
  fattal02_set_array (U, extent_u->width * extent_u->height, 0.0f);
  return;
}


/* smooth u using f at level */
static void
fattal02_smooth (gfloat              *U,
                 const GeglRectangle *extent_u,
                 gfloat              *F,
                 const GeglRectangle *extent_f)
{
  gint   iter;
  gfloat err;

  linbcg (extent_u->height,
          extent_u->width,
          F, U, 1, 0.001,
          BCG_STEPS, &iter, &err);

  /* pfstmo notes here that 'gauss relaxation is too slow'. */
}


static void
fattal02_calculate_defect (gfloat              *D,
                           const GeglRectangle *extent_d,
                           gfloat              *U,
                           const GeglRectangle *extent_u,
                           gfloat              *F,
                           const GeglRectangle *extent_f)
{
  guint sx = extent_f->width,
        sy = extent_f->height;
  guint x, y;

  for (y = 0; y < sy; ++y)
    {
      for (x = 0; x < sx; ++x)
        {
          guint w = (x     ==  0 ? 0 : x - 1),
                n = (y     ==  0 ? 0 : y - 1),
                s = (y + 1 == sy ? y : y + 1),
                e = (x + 1 == sx ? x : x + 1);

          _P (D, extent_d, x, y) = _P (F, extent_f, x, y) - (
                                      _P (U, extent_u, e, y) +
                                      _P (U, extent_u, w, y) +
                                      _P (U, extent_u, x, n) +
                                      _P (U, extent_u, x, s) -
                                      4.0 * _P (U, extent_u, x, y)
                                  );
        }
    }
}


static void
fattal02_solve_pde_multigrid (gfloat              *F,
                              const GeglRectangle *extent_f,
                              gfloat              *U,
                              const GeglRectangle *extent_u)
{
  guint xmax = extent_f->width,
        ymax = extent_f->height;

  gint i,	/* index for simple loops */
       k,	/* index for iterating through levels */
       k2;	/* index for iterating through levels in V-cycles */

  gint levels;

  gfloat **RHS, /* given function f restricted on levels */
         **IU,  /* approximate initial sollutions on levels */
         **VF;  /* target functions in cycles (approximate sollution error (uh - ~uh) ) */

  /* 1. restrict f to coarse-grid (by the way count the number of levels)
   *	  k=0: fine-grid = f
   *	  k=levels: coarsest-grid
   */
  {
    guint mins = MIN (xmax, ymax);
    levels = 0;

    while (mins >= MINS)
      {
        levels++;
        mins = mins / 2 + MODYF;
      }
  }

  RHS = g_new (gfloat*, levels + 1);
   IU = g_new (gfloat*, levels + 1);
   VF = g_new (gfloat*, levels + 1);

  RHS[0] = F;
   VF[0] = g_new (gfloat, xmax * ymax);
   IU[0] = g_new (gfloat, xmax * ymax);
  fattal02_copy_array (U, xmax * ymax, IU[0]);

  for (k = 0; k < levels; ++k)
    {
      RHS[k + 1] = g_new (gfloat, LEVEL_SIZE (extent_f, k + 1));
       IU[k + 1] = g_new (gfloat, LEVEL_SIZE (extent_f, k + 1));
       VF[k + 1] = g_new (gfloat, LEVEL_SIZE (extent_f, k + 1));

      /* restrict from level k to level k+1 (coarser-grid) */
      fattal02_restrict (RHS[k    ], &LEVEL_EXTENT (extent_f, k     ),
                         RHS[k + 1], &LEVEL_EXTENT (extent_f, k + 1));
    }

  /* 2. find exact solution at the coarsest-grid (k=levels) */
  fattal02_exact_solution (RHS[levels], &LEVEL_EXTENT (extent_f, levels),
                            IU[levels], &LEVEL_EXTENT (extent_f, levels));

  /* 3. nested iterations */
  for (k = levels - 1; k >= 0; --k)
    {
      guint cycle;

      /* 4. interpolate sollution from last coarse-grid to finer-grid
       * interpolate from level k+1 to level k (finer-grid)
       */
      fattal02_prolongate (IU[k + 1], &LEVEL_EXTENT (extent_f, k + 1),
                           IU[k    ], &LEVEL_EXTENT (extent_f, k    ));

      /* 4.1. first target function is the equation target function
       *      (following target functions are the defect)
       */
      fattal02_copy_array (RHS[k], LEVEL_SIZE (extent_f, k), VF[k]);

      /* 5. V-cycle (twice repeated) */
      for (cycle = 0; cycle < V_CYCLE; ++cycle)
        {
          /* 6. downward stroke of V */
          for (k2 = k; k2 < levels; ++k2)
            {
              gfloat *D;

              /* 7. pre-smoothing of initial sollution using target function
               *    zero for initial guess at smoothing
               *    (except for level k when iu contains prolongated result)
               */
              if (k2 != k)
                {
                  fattal02_set_array (IU[k2], LEVEL_SIZE (extent_f, k2), 0.0f);
                }

              for (i = 0; i < SMOOTH_IT; ++i)
                {
                  fattal02_smooth (IU[k2], &LEVEL_EXTENT (extent_f, k2),
                                   VF[k2], &LEVEL_EXTENT (extent_f, k2));
                }

              /* 8. calculate defect at level
               *    d[k2] = Lh * ~u[k2] - f[k2]
               */
              D = g_new (gfloat, LEVEL_SIZE (extent_f, k2));
              fattal02_calculate_defect (     D, &LEVEL_EXTENT (extent_f, k2),
                                         IU[k2], &LEVEL_EXTENT (extent_f, k2),
                                         VF[k2], &LEVEL_EXTENT (extent_f, k2));

              /* 9. restrict deffect as target function for next coarser-grid
               *    def -> f[k2+1]
               */
              fattal02_restrict (         D, &LEVEL_EXTENT (extent_f, k2    ),
                                 VF[k2 + 1], &LEVEL_EXTENT (extent_f, k2 + 1));
              g_free (D);
            }

          /* 10. solve on coarsest-grid (target function is the deffect)
           *     iu[levels] should contain sollution for
           *     the f[levels] - last deffect, iu will now be the correction
           */
          fattal02_exact_solution (VF[levels], &LEVEL_EXTENT (extent_f, levels),
                                   IU[levels], &LEVEL_EXTENT (extent_f, levels));

          /* 11. upward stroke of V */
          for (k2 = levels - 1; k2 >= k; --k2)
            {
              /* 12. interpolate correction from last coarser-grid to finer-grid
               *     iu[k2+1] -> cor
               */
              gfloat *C = g_new (gfloat, LEVEL_SIZE (extent_f, k2));
              fattal02_prolongate (IU[k2 + 1], &LEVEL_EXTENT (extent_f, k2 + 1),
                                            C, &LEVEL_EXTENT (extent_f, k2    ));

              /* 13. add interpolated correction to initial sollution at level k2 */
              fattal02_add_array (IU[k2], LEVEL_SIZE (extent_f, k2), C);
              g_free (C);

              /* 14. post-smoothing of current sollution using target function */
              for (i = 0; i < SMOOTH_IT; ++i)
                  fattal02_smooth (IU[k2], &LEVEL_EXTENT (extent_f, k2),
                                   VF[k2], &LEVEL_EXTENT (extent_f, k2));
            }

        } /*--- end of V-cycle */

    } /*--- end of nested iteration */

  /* 15. final sollution
   *     IU[0] contains the final sollution
   */

  fattal02_copy_array (IU[0], extent_f->width * extent_f->height, U);

  g_free (VF[0]);
  g_free (IU[0]);

  for (k = 1; k <= levels; ++k)
    {
      g_free (RHS[k]);
      g_free ( IU[k]);
      g_free ( VF[k]);
    }

  g_free (RHS);
  g_free ( IU);
  g_free ( VF);
}


static void
asolve (gulong n,
        gfloat b[],
        gfloat x[],
        gint   itrnsp)
{
  guint i;

  for (i = 0; i < n; ++i)
    x[i] = -4 * b[i];
}

static void
atimes (guint  rows,
        guint  cols,
        gfloat x[],
        gfloat res[],
        gint   itrnsp)
{
  guint r, c;

#define IDX(R,C) ((R) * cols + (C))

  for (r = 1; r < rows - 1; ++r)
    {
      for (c = 1; c < cols - 1; ++c)
        {
          res[IDX (r,c)] = x[IDX (r-1,c)] + x[IDX (r+1,c)] +
            x[IDX (r,c-1)] + x[IDX (r,c+1)] - 4*x[IDX (r,c)];
        }
    }

  for (r = 1; r < rows - 1; ++r)
    {
      res[IDX (r, 0)] =     x[IDX (r - 1, 0)] +
                            x[IDX (r + 1, 0)] +
                            x[IDX (r    , 1)] -
                        3 * x[IDX (r    , 0)];

      res[IDX (r, cols - 1)] =     x[IDX (r - 1, cols - 1)] +
                                   x[IDX (r + 1, cols - 1)] +
                                   x[IDX (r    , cols - 2)] -
                               3 * x[IDX (r    , cols - 1)];
    }

  for (c = 1; c < cols - 1; ++c)
    {
      res[IDX (0, c)] =     x[IDX (1, c    )] +
                            x[IDX (0, c - 1)] +
                            x[IDX (0, c + 1)] -
                        3 * x[IDX (0, c     )];

      res[IDX (rows - 1, c)] =     x[IDX (rows - 2, c    )] +
                                   x[IDX (rows - 1, c - 1)] +
                                   x[IDX (rows - 1, c + 1)] -
                               3 * x[IDX (rows - 1, c    )];
    }

  res[IDX (0       ,        0)] =     x[IDX (1       ,        0)] +
                                      x[IDX (0       ,        1)] -
                                  2 * x[IDX (0       ,        0)];
  res[IDX (rows - 1,        0)] =     x[IDX (rows - 2,        0)] +
                                      x[IDX (rows - 1,        1)] -
                                  2 * x[IDX (rows - 1,        0)];
  res[IDX (0       , cols - 1)] =     x[IDX (1       , cols - 1)] +
                                      x[IDX (0       , cols - 2)] -
                                  2 * x[IDX (0       , cols - 1)];
  res[IDX (rows - 1, cols - 1)] =     x[IDX (rows - 2, cols - 1)] +
                                      x[IDX (rows - 1, cols - 2)] -
                                  2 * x[IDX (rows - 1, cols - 1)];
}

static gfloat
snrm (gulong n,
      gfloat sx[],
      gint   itol)
{
  gulong i;

  if (itol <= 3)
    {
      gfloat ans = 0.0;
      for (i = 0; i < n; ++i)
          ans += sx[i] * sx[i];
      return sqrtf (ans);
    }
  else
    {
      gulong isamax = 0;
      for (i = 0; i < n; ++i)
        if (fabs (sx[i]) > fabs (sx[isamax]))
            isamax = i;
      return fabs (sx[isamax]);
    }
}


/**
 * Biconjugate Gradient Method
 * from Numerical Recipes in C
 */
static void
linbcg (guint   rows,
        guint   cols,
        gfloat  b[],
        gfloat  x[],
        gint    itol,
        gfloat  tol,
        gint    itmax,
        gint   *iter,
        gfloat *err)
{
  guint  n = rows * cols;

  gulong j;
  gfloat ak,akden,bk,bkden,bknum,bnrm,dxnrm,xnrm,zm1nrm,znrm;
  gfloat *p,*pp,*r,*rr,*z,*zz;

  /* To remove warning about potetial uninitialized use */
  bkden = 1;

  p  = g_new (gfloat, n);
  pp = g_new (gfloat, n);
  r  = g_new (gfloat, n);
  rr = g_new (gfloat, n);
  z  = g_new (gfloat, n);
  zz = g_new (gfloat, n);

  *iter=0;
  atimes (rows, cols, x, r, 0);
  for (j = 0; j < n; ++j)
    {
       r[j] = b[j] - r[j];
      rr[j] = r[j];
    }

  atimes (rows, cols, r, rr, 0);       /* minimum residual */
  znrm = 1.0;

  if (itol == 1)
    {
      bnrm = snrm (n, b, itol);
    }
  else if (itol == 2)
    {
      asolve (n, b, z, 0);
      bnrm = snrm (n, z, itol);
    }
  else if (itol == 3 || itol == 4)
    {
      asolve (n, b, z, 0);
      bnrm = snrm (n, z, itol);
      asolve (n, r, z, 0);
      znrm = snrm (n, z, itol);
    }
  else
    {
      g_warning ("illegal itol in linbcg");
    }

  asolve (n, r, z, 0);

  while (*iter <= itmax)
    {
      ++(*iter);

      zm1nrm = znrm;
      asolve (n, rr, zz, 1);
      for (bknum = 0.0, j = 0; j < n; ++j)
        {
          bknum += z[j] * rr[j];
        }

      if (*iter == 1)
        {
          for (j = 0; j < n; ++j)
            {
               p[j] =  z[j];
              pp[j] = zz[j];
            }
        }
      else
        {
          bk = bknum / bkden;

          for (j = 0; j < n; ++j)
            {
               p[j] = bk *  p[j] +  z[j];
              pp[j] = bk * pp[j] + zz[j];
            }
        }

      bkden = bknum;
      atimes (rows, cols, p, z, 0);

      for (akden = 0.0, j = 0; j < n; ++j)
        {
          akden += z[j] * pp[j];
        }

      ak = bknum / akden;
      atimes (rows, cols, pp, zz, 1);

      for (j = 0; j < n; ++j)
        {
           x[j] += ak *  p[j];
           r[j] -= ak *  z[j];
          rr[j] -= ak * zz[j];
        }

      asolve (n, r, z, 0);

      if (itol == 1 || itol == 2)
        {
          znrm = 1.0;
          *err = snrm (n, r, itol) / bnrm;
        }
      else if (itol == 3 || itol == 4)
        {
          znrm = snrm (n, z, itol);

          if (fabs (zm1nrm - znrm) > EPS * znrm)
            {
              dxnrm = fabs (ak) * snrm (n, p, itol);
              *err = znrm / fabs (zm1nrm - znrm) * dxnrm;
            }
          else
            {
              *err = znrm / bnrm;
              continue;
            }

          xnrm = snrm (n, x, itol);
          if (*err <= 0.5 * xnrm)
            {
              *err /= xnrm;
            }
          else
            {
              *err=znrm/bnrm;
              continue;
            }
        }

      if (*err <= tol)
        break;
    }

  g_free (p);
  g_free (pp);
  g_free (r);
  g_free (rr);
  g_free (z);
  g_free (zz);
}


/* Downscale the input buffer by a factor of two. Extent describes the input
 * buffer. Assumes a pixel stride of 1, as we're really only dealing with
 * luminance. Output should be preallocated with a size that is half of the
 * input.
 */
static void
fattal02_downsample (const gfloat        *input,
                     const GeglRectangle *extent,
                     gfloat              *output)
{
  guint x, y, width, height;
  g_return_if_fail (input);
  g_return_if_fail (extent);
  g_return_if_fail (output);

  width  = extent->width  / 2,
  height = extent->height / 2;

  g_return_if_fail (width  > 0);
  g_return_if_fail (height > 0);

  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          gfloat p = 0.0f;

          /* Sum the 4 pixels from the input which directly contribute to the
           * output, and divide by four.
           */
          p += input[(2 * y + 0) * extent->width + (2 * x + 0)];
          p += input[(2 * y + 0) * extent->width + (2 * x + 1)];
          p += input[(2 * y + 1) * extent->width + (2 * x + 0)];
          p += input[(2 * y + 1) * extent->width + (2 * x + 1)];

          output [y * width + x] = p / 4.0f;
        }
    }
}


/* Blur the input buffer with a one pixel radius. Output should be
 * preallocated with the same size as the input buffer. This must perform
 * correctly when input and output alias.
 */
static void
fattal02_gaussian_blur (const gfloat        *input,
                        const GeglRectangle *extent,
                        gfloat              *output)
{
  const guint  width  = extent->width,
               height = extent->height,
               size   = width * height;
  guint        x, y;
  gfloat      *temp;

  g_return_if_fail (input);
  g_return_if_fail (extent);
  g_return_if_fail (output);
  g_return_if_fail (size > 0);

  temp   = g_new (gfloat, size);

  /* horizontal blur */
  for (y = 0; y < height; ++y)
    {
      for (x = 1; x < width - 1; ++x)
        {
          gfloat p  = 2 * input[x     + y * width];
          p        +=     input[x - 1 + y * width];
          p        +=     input[x + 1 + y * width];

          temp[x + y * extent->width] = p / 4.0f;
        }

      temp[0         + y * width] = (3 * input[0         + y * width] +
                                         input[1         + y * width]) / 4.0f;
      temp[width - 1 + y * width] = (3 * input[width - 1 + y * width] +
                                         input[width - 2 + y * width]) / 4.0f;
    }

  /* vertical blur */
  for (x = 0; x < width; ++x)
    {
      for (y = 1; y < height - 1; ++y)
        {
          gfloat p  = 2 * temp[x +      y  * width];
          p        +=     temp[x + (y - 1) * width];
          p        +=     temp[x + (y + 1) * width];

          output[x + y * width] = p / 4.0f;
        }

      output[x +           0  * width] = (3 * temp[x +           0  * width] +
                                              temp[x +           1  * width]) / 4.0f;
      output[x + (height - 1) * width] = (3 * temp[x + (height - 1) * width] +
                                              temp[x + (height - 2) * width]) / 4.0f;
    }

  g_free (temp);
}


static void
fattal02_create_gaussian_pyramids (const gfloat         *zero,
                                   const GeglRectangle  *extent,
                                   gfloat              **pyramid,
                                   gint                  levels)
{
  gint           i;
  gfloat        *blur;
  GeglRectangle  level_extent = *extent;

  /* Copy the first level of the pyramid into place */
  pyramid[0] = g_new (gfloat, level_extent.width * level_extent.height);
  for (i = 0; i < level_extent.width * level_extent.height; ++i)
    {
      pyramid[0][i] = zero[i];
    }

  /* Establish a temporary blur buffer. The allocated memory will be used for
   * progressively smaller levels, and we don't free this until the end.
   */
  blur = g_new (gfloat, level_extent.width * level_extent.height);
  fattal02_gaussian_blur (pyramid[0], &level_extent, blur);

  for (i = 1; i < levels; ++i)
    {
      level_extent.width  /= 2;
      level_extent.height /= 2;

      g_return_if_fail (level_extent.width  >= MINIMUM_PYRAMID);
      g_return_if_fail (level_extent.height >= MINIMUM_PYRAMID);

      /* Downsample the blurred buffer into the pyramid */
      pyramid[i] = g_new (gfloat, LEVEL_SIZE (extent, i));
      fattal02_downsample (blur, &LEVEL_EXTENT (extent, i - 1), pyramid[i]);

      /* Blur the current level over the blur buffer */
      fattal02_gaussian_blur (pyramid[i], &level_extent, blur);
    }

  g_free (blur);
}


/********************************************************************/

static gfloat
fattal02_calculate_gradients (const gfloat        *input,   /* H */
                              const GeglRectangle *extent,  /*  */
                              gfloat              *output,  /* G */
                              gint                 k)
{
  guint  width   = extent->width,
         height  = extent->height;
  gfloat divider = powf (2.0f, k + 1),
         average = 0.0f;

  guint  x, y;

  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          gfloat gx, gy;
          gint   w, n, e, s;

          w = (x     ==      0 ? 0 : x - 1);
          n = (y     ==      0 ? 0 : y - 1);
          s = (y + 1 == height ? y : y + 1);
          e = (x + 1 == width  ? x : x + 1);

          gx = (input[w + y * width] - input[e + y * width]) / divider;
          gy = (input[x + s * width] - input[x + n * width]) / divider;

          output[x + y * width] = sqrtf (gx * gx + gy * gy);
          average += output[x + y * width];
        }
    }

  return average / (width * height);
}


/********************************************************************/

static void
fattal02_upsample (const gfloat        *input,
                   const GeglRectangle *extent,
                   gfloat              *output)
{
  guint  width_i = extent->width,
        height_i = extent->height,
         width_o =  width_i * 2,
        height_o = height_i * 2;
  guint x_o, y_o;

  for (y_o = 0; y_o < height_o; ++y_o)
    {
      for (x_o = 0; x_o < width_o; ++x_o)
        {
          guint x_i = x_o / 2,
            y_i = y_o / 2;

          x_i = (x_i <  width_i) ? x_i :  width_i - 1;
          y_i = (y_i < height_i) ? y_i : height_i - 1;

          output[x_o + y_o * width_o] = input[x_i + y_i * width_i];
        }
    }
}


static void
fattal02_FI_matrix (gfloat               *FI,
                    const GeglRectangle  *extent,
                    gfloat              **gradients,
                    const gfloat         *averages,
                    const gint            levels,
                    const gfloat          alfa,
                    const gfloat          beta,
                    const gfloat          noise)
{
  GeglRectangle   level_extent = *extent;
  gint            i;
  gfloat        **fi;

  level_extent.width  = LEVEL_WIDTH  (extent, levels - 1);
  level_extent.height = LEVEL_HEIGHT (extent, levels - 1);

  fi = g_new (gfloat*, levels);

  fi[levels - 1] = g_new (gfloat, level_extent.width * level_extent.height);

  for (i = 0; i < level_extent.width * level_extent.height; ++i)
    {
      fi[levels - 1][i] = 1.0f;
    }

  for (i = levels - 1; i >= 0; --i)
    {
      gint x, y;

      level_extent.width  = LEVEL_WIDTH  (extent, i);
      level_extent.height = LEVEL_HEIGHT (extent, i);

      for (y = 0; y < level_extent.height; ++y)
          for (x = 0; x < level_extent.width; ++x)
            {
              gfloat grad  = gradients[i][x + y * level_extent.width],
                     a     = alfa * averages[i],
                     value = 1.0f;

              if (grad > 1e-4f)
                value = a / (grad + noise) * powf ((grad + noise) / a, beta);
              fi[i][x + y * level_extent.width] *= value;
            }

      /* create next level */
      if (i > 1)
        {
          level_extent.width  = LEVEL_WIDTH  (extent, i - 1);
          level_extent.height = LEVEL_HEIGHT (extent, i - 1);

          fi[i - 1] = g_new (gfloat,
                             level_extent.width * level_extent.height);
        }
      else
        {
          fi[0] = FI;               /* highest level -> result */
        }

      if (i > 0)
        {
          /* upsample to next level */
          fattal02_upsample      (fi[i    ], &LEVEL_EXTENT (extent, i    ), fi[i - 1]);
          fattal02_gaussian_blur (fi[i - 1], &LEVEL_EXTENT (extent, i - 1), fi[i - 1]);
        }
    }

  /* Careful not to delete the result memory in fi[0] */
  for (i = 1; i < levels; ++i)
    g_free (fi[i]);

  g_free (fi);
}

/********************************************************************/


static int
fattal02_float_cmp (const void *_a,
                    const void *_b)
{
  const gfloat a = *(gfloat *)_a,
               b = *(gfloat *)_b;

  if (a < b) return -1;
  if (a > b) return  1;
             return  0;
}


static void
fattal02_find_percentiles (const gfloat *array,
                           const guint   size,
                           const gfloat  min_percent,
                           gfloat       *min_value,
                           const gfloat  max_percent,
                           gfloat       *max_value)
{
  guint   i;
  gfloat *sorting;

  g_return_if_fail (min_percent <= max_percent);
  g_return_if_fail (min_percent >= 0.0f && min_percent <= 1.0f);
  g_return_if_fail (max_percent >= 0.0f && max_percent <= 1.0f);

  sorting = g_new (gfloat, size);
  for (i = 0; i < size; ++i)
    {
      sorting[i] = array[i];
    }

  qsort (sorting, size, sizeof (sorting[0]), fattal02_float_cmp);

  *min_value = sorting[(guint)(min_percent * size)];
  *max_value = sorting[(guint)(max_percent * size)];
  g_free (sorting);
}

/********************************************************************/

static void
fattal02_tonemap (const gfloat        *input,   /* Y */
                  const GeglRectangle *extent,
                  gfloat              *output,  /* L */
                  gfloat               alfa,
                  gfloat               beta,
                  gfloat               noise)
{
  gint     height = extent->height,
           width  = extent->width,
           size   = height * width;
  gint     x, y, i;
  gfloat  *H, *FI, *Gx, *Gy, *divergence, *U;
  gint     levels;
  gfloat **pyramid;
  gfloat **gradient,
          *averages;

  /* find max & min values, normalize to range 0..100 and take logarithm */
  {
    gfloat min_input = G_MAXFLOAT,
           max_input = G_MINFLOAT;

    for (i = 0; i < size; ++i)
      {
        min_input = MIN (min_input, input[i]);
        max_input = MAX (max_input, input[i]);
      }
    g_return_if_fail (min_input <= max_input);

    H = g_new (gfloat, size);
    for (i = 0; i < size; ++i)
      {
        H[i] = log (100.0f * input[i] / max_input + 1e-4f);
      }
  }

  GEGL_NOTE (GEGL_DEBUG_PROCESS, "calculating attenuation matrix");

  /* create gaussian pyramids */
  {
    gint min_size = MIN (extent->width, extent->height);

    for (levels = 0; min_size / 2 >= MINIMUM_PYRAMID; )
      {
        ++levels;
        min_size /= 2;
      }

    pyramid = g_new (gfloat*, levels);
    fattal02_create_gaussian_pyramids (H, extent, pyramid, levels);
  }

  /* calculate gradients and its average values on pyramid levels */
  gradient = g_new (gfloat*, levels);
  averages = g_new (gfloat,  levels);

  for (i = 0; i < levels; ++i)
    {
      gradient[i] = g_new (gfloat, LEVEL_SIZE (extent, i));
      averages[i] = fattal02_calculate_gradients (pyramid[i],
                                                  &LEVEL_EXTENT (extent, i),
                                                  gradient[i],
                                                  i);
    }

  /* calculate fi matrix */
  FI = g_new (gfloat, size);
  fattal02_FI_matrix (FI, extent, gradient, averages, levels,
                      alfa, beta, noise);

  /* attenuate gradients */
  Gx = g_new (gfloat, size);
  Gy = g_new (gfloat, size);

  for (y = 0; y < extent->height; ++y)
    {
      for (x = 0; x < extent->width; ++x)
        {
          guint s = (y + 1 == height ? y : y + 1),
            e = (x + 1 ==  width ? x : x + 1);

          Gx[x + y * width] = ( H[e + y * width] - H[x + y * width]) *
                               FI[x + y * width];
          Gy[x + y * width] = ( H[x + s * width] - H[x + y * width]) *
                               FI[x + y * width];
        }
    }

  GEGL_NOTE (GEGL_DEBUG_PROCESS, "compressing gradients");

  /* calculate divergence */
  divergence = g_new (gfloat, size);
  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
        {
          divergence[x + y * width] = Gx[x + y * width] + Gy[x + y * width];
          if (x > 0) divergence[x + y * width] -= Gx[x - 1 + (y    ) * width];
          if (y > 0) divergence[x + y * width] -= Gy[x     + (y - 1) * width];
        }
    }

  GEGL_NOTE (GEGL_DEBUG_PROCESS, "recovering image");

  /* solve pde and exponentiate (ie recover compressed image) */
  U = g_new (gfloat, size);
  fattal02_solve_pde_multigrid (divergence, extent, U, extent);

  for (i = 0; i < size; ++i)
    output[i] = expf (U[i]) - 1e-4f;

  {
    gfloat min, max, range;

    /* remove percentile of min and max values and renormalize */
    fattal02_find_percentiles (output, size,
                               0.001f, &min,
                               0.995f, &max);
    range = max - min;

    for (i = 0; i < size; ++i)
      {
        output[i] = (output[i] - min) / range;
        if (output[i] <= 0.0f)
            output[i] = 1e-4f;
      }
  }

  /* clean up */
  g_free (H);
  for (i = 0; i < levels; ++i)
    {
      g_free (  pyramid[i]);
      g_free (gradient[i]);
    }

  g_free (pyramid);
  g_free (gradient);
  g_free (averages);
  g_free (FI);
  g_free (Gx);
  g_free (Gy);
  g_free (divergence);
  g_free (U);
}


static void
fattal02_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format (OUTPUT_FORMAT));
  gegl_operation_set_format (operation, "output", babl_format (OUTPUT_FORMAT));
}


static GeglRectangle
fattal02_get_required_for_output (GeglOperation       *operation,
                                  const gchar         *input_pad,
                                  const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}


static GeglRectangle
fattal02_get_cached_region (GeglOperation       *operation,
                            const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}


static gboolean
fattal02_process (GeglOperation       *operation,
                  GeglBuffer          *input,
                  GeglBuffer          *output,
                  const GeglRectangle *result,
                  gint                 level)
{
  const GeglChantO *o     = GEGL_CHANT_PROPERTIES (operation);
  gfloat            noise;

  const gint  pix_stride = 3; /* RGBA */
  gfloat     *lum_in,
             *lum_out,
             *pix;
  gint        i;

  g_return_val_if_fail (operation, FALSE);
  g_return_val_if_fail (input, FALSE);
  g_return_val_if_fail (output, FALSE);
  g_return_val_if_fail (result, FALSE);

  g_return_val_if_fail (babl_format_get_n_components (babl_format (OUTPUT_FORMAT)) == pix_stride, FALSE);

  /* Adjust noise floor if not set by the user */
  if (o->noise == 0.0)
    {
      noise = o->alpha * 0.1;
    }
  else
    {
      noise = o->noise;
    }

  /* Obtain the pixel data */
  lum_in  = g_new (gfloat, result->width * result->height);
  lum_out = g_new (gfloat, result->width * result->height);

  gegl_buffer_get (input, result, 1.0, babl_format ("Y float"),
                   lum_in, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  pix = g_new (gfloat, result->width * result->height * pix_stride);
  gegl_buffer_get (input, result, 1.0, babl_format (OUTPUT_FORMAT),
                   pix, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  fattal02_tonemap (lum_in, result, lum_out, o->alpha, o->beta, noise);

  for (i = 0; i < result->width * result->height * pix_stride; ++i)
    {
      pix[i] = (powf (pix[i] / lum_in[i / pix_stride],
                      o->saturation) *
                lum_out[i / pix_stride]);
    }

  gegl_buffer_set (output, result, 0, babl_format (OUTPUT_FORMAT), pix,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (pix);
  g_free (lum_out);
  g_free (lum_in);
  return TRUE;
}


/*
 */
static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = fattal02_process;

  operation_class->prepare                 = fattal02_prepare;
  operation_class->get_required_for_output = fattal02_get_required_for_output;
  operation_class->get_cached_region       = fattal02_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
  "name"       , "gegl:fattal02",
  "categories" , "tonemapping",
  "description",
        _("Adapt an image, which may have a high dynamic range, for "
	  "presentation using a low dynamic range. This operator attenuates "
          "the magnitudes of local image gradients, producing luminance "
          "within the range 0.0-1.0"),
        NULL);
}

#endif


