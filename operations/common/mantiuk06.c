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
 * Copyright 2010      Danny Robson      <danny@blubinc.net>
 * (pfstmo)  2007      Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
 *           2007-2008 Ed Brambley       <E.J.Brambley@damtp.cam.ac.uk>
 *                     Lebed Dmytry
 *                     Radoslaw Mantiuk  <radoslaw.mantiuk@gmail.com>
 *                     Rafal Mantiuk     <mantiuk@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (contrast, _("Contrast"),
                  0.0, 1.0, 0.1,
                  _("The amount of contrast compression"))
gegl_chant_double (saturation, _("Saturation"),
                  0.0, 2.0, 0.8,
                  _("Global colour saturation factor"))
gegl_chant_double (detail, _("Detail"),
                  1.0, 99.0, 1.0,
                  _("Level of emphasis on image gradient details"))


#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "mantiuk06.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_OPENMP
#define _OMP(x) _Pragma(#x)
#else
#define _OMP(x) /* OMP disabled: "#x" */
#endif

/* Common return codes for operators */
#define PFSTMO_OK 1             /* Successful */
#define PFSTMO_ABORTED -1       /* User aborted (from callback) */
#define PFSTMO_ERROR -2         /* Failed, encountered error */

/* Return codes for the progress_callback */
#define PFSTMO_CB_CONTINUE  1
#define PFSTMO_CB_ABORT    -1

typedef struct pyramid_s {
  guint              rows;
  guint              cols;
  gfloat            *Gx;
  gfloat            *Gy;
  struct pyramid_s  *next;
  struct pyramid_s  *prev;
} pyramid_t;


#define PYRAMID_MIN_PIXELS 3
#define LOOKUP_W_TO_R 107

typedef int (*pfstmo_progress_callback)(int progress);


static void        mantiuk06_contrast_equalization            (pyramid_t                       *pp,
                                                               const gfloat                     contrastFactor);
static void        mantiuk06_transform_to_luminance           (pyramid_t                       *pp,
                                                               gfloat                   *const  x,
                                                               pfstmo_progress_callback         progress,
                                                               const gboolean                   bcg,
                                                               const int                        itmax,
                                                               const gfloat                     tol);
static gfloat*     mantiuk06_matrix_alloc                     (const guint                      size);
static void        mantiuk06_matrix_free                      (gfloat                          *m);
static void        mantiuk06_matrix_zero                      (const guint                      n,
                                                               gfloat     *const                m);
static void        mantiuk06_matrix_copy                      (const guint                      n,
                                                               const gfloat *const              a,
                                                               gfloat       *const              b);
static void        mantiuk06_matrix_subtract                  (const guint                      n,
                                                               const gfloat *const              a,
                                                               gfloat *const                    b);
static void        mantiuk06_matrix_multiply_const            (const guint                      n,
                                                               gfloat       *const              a,
                                                               const gfloat                     val);
static void        mantiuk06_matrix_divide                    (const guint                      n,
                                                               const gfloat *const              a,
                                                               gfloat *const                    b);
static gfloat      mantiuk06_matrix_dot_product               (const guint                      n,
                                                               const gfloat *const              a,
                                                               const gfloat *const              b);
static void        mantiuk06_calculate_and_add_divergence     (const int                        rows,
                                                               const int                        cols,
                                                               const gfloat *const              Gx,
                                                               const gfloat *const              Gy,
                                                               gfloat       *const              divG);
static void        mantiuk06_pyramid_calculate_divergence_sum (pyramid_t                       *pyramid,
                                                               gfloat                          *divG_sum);
static void        mantiuk06_calculate_scale_factor           (const int                        n,
                                                               const gfloat *const              G,
                                                               gfloat       *const              C);
static void        mantiuk06_pyramid_calculate_scale_factor   (pyramid_t                       *pyramid,
                                                               pyramid_t                       *pC);
static void        mantiuk06_scale_gradient                   (const int                        n,
                                                               gfloat       *const              G,
                                                               const gfloat *const              C);
static void        mantiuk06_pyramid_scale_gradient           (pyramid_t                       *pyramid,
                                                               pyramid_t                       *pC);
static void        mantiuk06_pyramid_free                     (pyramid_t                       *pyramid);
static pyramid_t*  mantiuk06_pyramid_allocate                 (const int                        cols,
                                                               const int                        rows);
static void        mantiuk06_calculate_gradient               (const int                        cols,
                                                               const int                        rows,
                                                               const gfloat *const              lum,
                                                               gfloat       *const              Gx,
                                                               gfloat       *const              Gy);
static void        mantiuk06_pyramid_calculate_gradient       (pyramid_t                       *pyramid,
                                                               gfloat                          *lum);
static void        mantiuk06_solveX                           (const gint                       n,
                                                               const gfloat *const              b,
                                                               gfloat       *const              x);
static void        mantiuk06_multiplyA                        (pyramid_t                       *px,
                                                               pyramid_t                       *pyramid,
                                                               const gfloat *const              x,
                                                               gfloat       *const              divG_sum);
static void        mantiuk06_linbcg                           (pyramid_t                       *pyramid,
                                                               pyramid_t                       *pC,
                                                               const gfloat *const              b,
                                                               gfloat       *const              x,
                                                               const int                        itmax,
                                                               const gfloat                     tol,
                                                               pfstmo_progress_callback         progress_cb);
static void        mantiuk06_lincg                            (pyramid_t                       *pyramid,
                                                               pyramid_t                       *pC,
                                                               const gfloat *const              b,
                                                               gfloat       *const              x,
                                                               const int                        itmax,
                                                               const gfloat                     tol,
                                                               pfstmo_progress_callback         progress_cb);
static gfloat      mantiuk06_lookup_table                     (const int                        n,
                                                               const gfloat *const              in_tab,
                                                               const gfloat *const              out_tab,
                                                               const gfloat                     val);
static void        mantiuk06_transform_to_R                   (const int                        n,
                                                               gfloat     *const                G);
static void        mantiuk06_pyramid_transform_to_R           (pyramid_t                       *pyramid);
static void        mantiuk06_transform_to_G                   (const gint                       n,
                                                               gfloat     *const                R);
static void        mantiuk06_pyramid_transform_to_G           (pyramid_t                       *pyramid);
static void        mantiuk06_pyramid_gradient_multiply        (pyramid_t                       *pyramid,
                                                               const gfloat                     val);


static const gchar *OUTPUT_FORMAT = "RGBA float";

static gfloat W_table[] =
{
     0.000000,     0.010000,    0.021180,    0.031830,    0.042628,
     0.053819,     0.065556,    0.077960,    0.091140,    0.105203,
     0.120255,     0.136410,    0.153788,    0.172518,    0.192739,
     0.214605,     0.238282,    0.263952,    0.291817,    0.322099,
     0.355040,     0.390911,    0.430009,    0.472663,    0.519238,
     0.570138,     0.625811,    0.686754,    0.753519,    0.826720,
     0.907041,     0.995242,    1.092169,    1.198767,    1.316090,
     1.445315,     1.587756,    1.744884,    1.918345,    2.109983,
     2.321863,     2.556306,    2.815914,    3.103613,    3.422694,
     3.776862,     4.170291,    4.607686,    5.094361,    5.636316,
     6.240338,     6.914106,    7.666321,    8.506849,    9.446889,
    10.499164,    11.678143,   13.000302,   14.484414,   16.151900,
    18.027221,    20.138345,   22.517282,   25.200713,   28.230715,
    31.655611,    35.530967,   39.920749,   44.898685,   50.549857,
    56.972578,    64.280589,   72.605654,   82.100619,   92.943020,
   105.339358,   119.530154,  135.795960,  154.464484,  175.919088,
   200.608905,   229.060934,  261.894494,  299.838552,  343.752526,
   394.651294,   453.735325,  522.427053,  602.414859,  695.706358,
   804.693100,   932.229271, 1081.727632, 1257.276717, 1463.784297,
  1707.153398,  1994.498731, 2334.413424, 2737.298517, 3215.770944,
  3785.169959,  4464.187290, 5275.653272, 6247.520102, 7414.094945,
  8817.590551, 10510.080619
};

static gfloat R_table[] =
{
  0.000000, 0.009434, 0.018868, 0.028302, 0.037736,
  0.047170, 0.056604, 0.066038, 0.075472, 0.084906,
  0.094340, 0.103774, 0.113208, 0.122642, 0.132075,
  0.141509, 0.150943, 0.160377, 0.169811, 0.179245,
  0.188679, 0.198113, 0.207547, 0.216981, 0.226415,
  0.235849, 0.245283, 0.254717, 0.264151, 0.273585,
  0.283019, 0.292453, 0.301887, 0.311321, 0.320755,
  0.330189, 0.339623, 0.349057, 0.358491, 0.367925,
  0.377358, 0.386792, 0.396226, 0.405660, 0.415094,
  0.424528, 0.433962, 0.443396, 0.452830, 0.462264,
  0.471698, 0.481132, 0.490566, 0.500000, 0.509434,
  0.518868, 0.528302, 0.537736, 0.547170, 0.556604,
  0.566038, 0.575472, 0.584906, 0.594340, 0.603774,
  0.613208, 0.622642, 0.632075, 0.641509, 0.650943,
  0.660377, 0.669811, 0.679245, 0.688679, 0.698113,
  0.707547, 0.716981, 0.726415, 0.735849, 0.745283,
  0.754717, 0.764151, 0.773585, 0.783019, 0.792453,
  0.801887, 0.811321, 0.820755, 0.830189, 0.839623,
  0.849057, 0.858491, 0.867925, 0.877358, 0.886792,
  0.896226, 0.905660, 0.915094, 0.924528, 0.933962,
  0.943396, 0.952830, 0.962264, 0.971698, 0.981132,
  0.990566, 1.000000
};


/* upsample the matrix
 * upsampled matrix is twice bigger in each direction than data[]
 * res should be a pointer to allocated memory for bigger matrix
 * cols and rows are the dimmensions of the output matrix
 */
static void
mantiuk06_matrix_upsample (const gint          outCols,
                           const gint          outRows,
                           const gfloat *const in,
                           gfloat       *const out)
{
  const int inRows = outRows/2;
  const int inCols = outCols/2;
  gint      x, y;

  /* Transpose of experimental downsampling matrix (theoretically the
   * correct thing to do)
   */

  const gfloat dx = (gfloat)inCols / ((gfloat)outCols);
  const gfloat dy = (gfloat)inRows / ((gfloat)outRows);
  const gfloat factor = 1.0f / (dx*dy); /* This gives a genuine upsampling
                                         * matrix, not the transpose of the
                                         * downsampling matrix
                                         */
  /* const gfloat factor = 1.0f; */     /* Theoretically, this should be the
                                         * best.
                                         */

  _OMP (omp parallel for schedule(static))
  for (y = 0; y < outRows; y++)
    {
      const gfloat sy  = y * dy;
      const gint   iy1 =      (  y   * inRows) / outRows;
      const gint   iy2 = MIN (((y+1) * inRows) / outRows, inRows-1);

      for (x = 0; x < outCols; x++)
        {
          const gfloat sx  = x * dx;
          const gint   ix1 =      (  x    * inCols) / outCols;
          const gint   ix2 =  MIN (((x+1) * inCols) / outCols, inCols-1);

          out[x + y*outCols] = (
            ((ix1+1) - sx)*((iy1+1 - sy)) * in[ix1 + iy1*inCols] +
            ((ix1+1) - sx)*(sy+dy - (iy1+1)) * in[ix1 + iy2*inCols] +
            (sx+dx - (ix1+1))*((iy1+1 - sy)) * in[ix2 + iy1*inCols] +
            (sx+dx - (ix1+1))*(sy+dx - (iy1+1)) * in[ix2 + iy2*inCols])*factor;
        }
    }
}


/* downsample the matrix */
static void
mantiuk06_matrix_downsample (const gint          inCols,
                             const gint          inRows,
                             const gfloat *const data,
                             gfloat       *const res)
{
  const int outRows = inRows / 2;
  const int outCols = inCols / 2;
  gint      x, y, i, j;

  const gfloat dx = (gfloat)inCols / ((gfloat)outCols);
  const gfloat dy = (gfloat)inRows / ((gfloat)outRows);

  /* New downsampling by Ed Brambley:
   * Experimental downsampling that assumes pixels are square and
   * integrates over each new pixel to find the average value of the
   * underlying pixels.
   *
   * Consider the original pixels laid out, and the new (larger)
   * pixels layed out over the top of them.  Then the new value for
   * the larger pixels is just the integral over that pixel of what
   * shows through; i.e., the values of the pixels underneath
   * multiplied by how much of that pixel is showing.
   *
   * (ix1, iy1) is the coordinate of the top left visible pixel.
   * (ix2, iy2) is the coordinate of the bottom right visible pixel.
   * (fx1, fy1) is the fraction of the top left pixel showing.
   * (fx2, fy2) is the fraction of the bottom right pixel showing.
   */

  const gfloat normalize = 1.0f/(dx*dy);
  _OMP (omp parallel for schedule(static))
  for (y = 0; y < outRows; y++)
    {
      const gint   iy1 = (  y   * inRows) / outRows;
      const gint   iy2 = ((y+1) * inRows) / outRows;
      const gfloat fy1 = (iy1+1) - y * dy;
      const gfloat fy2 = (y+1) * dy - iy2;

      for (x = 0; x < outCols; x++)
        {
          const gint   ix1 = (  x   * inCols) / outCols;
          const gint   ix2 = ((x+1) * inCols) / outCols;
          const gfloat fx1 = (ix1+1) - x * dx;
          const gfloat fx2 = (x+1) * dx - ix2;

          gfloat pixVal = 0.0f;
          gfloat factorx, factory;

          for (i = iy1; i <= iy2 && i < inRows; i++)
            {
              if (i == iy1)
                {
                  /* We're just getting the bottom edge of this pixel */
                  factory = fy1;
                }
              else if (i == iy2)
                {
                  /* We're just gettting the top edge of this pixel */
                  factory = fy2;
                }
              else
                {
                  /* We've got the full height of this pixel */
                  factory = 1.0f;
                }

              for (j = ix1; j <= ix2 && j < inCols; j++)
                {
                  if (j == ix1)
                    {
                      /* We've just got the right edge of this pixel */
                      factorx = fx1;
                    }
                  else if (j == ix2)
                    {
                      /* We've just got the left edge of this pixel */
                      factorx = fx2;
                    }
                  else
                    {
                      /* We've got the full width of this pixel */
                      factorx = 1.0f;
                    }

                  pixVal += data[j + i*inCols] * factorx * factory;
                }
            }

          /* Normalize by the area of the new pixel */
          res[x + y * outCols] = pixVal * normalize;
        }
    }
}


/* return = a - b */
static inline void
mantiuk06_matrix_subtract (const guint         n,
                           const gfloat *const a,
                           gfloat       *const b)
{
  guint i;

  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
    b[i] = a[i] - b[i];
}

/* copy matix a to b, return = a  */
static inline void
mantiuk06_matrix_copy (const guint         n,
                       const gfloat *const a,
                       gfloat       *const b)
{
  memcpy (b, a, sizeof (gfloat) * n);
}

/* multiply matrix a by scalar val */
static inline void
mantiuk06_matrix_multiply_const (const guint         n,
                                 gfloat       *const a,
                                 const gfloat        val)
{
  guint i;
  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
    a[i] *= val;
}

/* b = a[i] / b[i] */
static inline void
mantiuk06_matrix_divide (const guint         n,
                         const gfloat *const a,
                         gfloat       *const b)
{
  guint i;
  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
      b[i] = a[i] / b[i];
}


static inline gfloat *
mantiuk06_matrix_alloc (guint size)
{
  return g_new (gfloat, size);
}


static inline void
mantiuk06_matrix_free (gfloat *m)
{
  g_return_if_fail (m);
  g_free (m);
}

/* multiply vector by vector (each vector should have one dimension equal to 1) */
static inline gfloat
mantiuk06_matrix_dot_product (const guint         n,
                              const gfloat *const a,
                              const gfloat *const b)
{
  gfloat val = 0;
  guint j;

  _OMP (omp parallel for reduction(+:val) schedule(static))
  for (j = 0; j < n; j++)
    val += a[j] * b[j];

  return val;
}

/* set zeros for matrix elements */
static inline void
mantiuk06_matrix_zero (guint   n,
                       gfloat *m)
{
  memset(m, 0, n * sizeof (gfloat));
}

/* calculate divergence of two gradient maps (Gx and Gy)
 * divG(x,y) = Gx(x,y) - Gx(x-1,y) + Gy(x,y) - Gy(x,y-1)
 */
static inline void
mantiuk06_calculate_and_add_divergence (const gint          cols,
                                        const gint          rows,
                                        const gfloat *const Gx,
                                        const gfloat *const Gy,
                                        gfloat       *const divG)
{
  gint ky, kx;

  _OMP (omp parallel for schedule(static))
  for (ky = 0; ky < rows; ky++)
    {
      for (kx = 0; kx<cols; kx++)
        {
          gfloat divGx, divGy;
          const int idx = kx + ky*cols;

          if (kx == 0)
            divGx = Gx[idx];
          else
            divGx = Gx[idx] - Gx[idx-1];

          if (ky == 0)
            divGy = Gy[idx];
          else
            divGy = Gy[idx] - Gy[idx - cols];

          divG[idx] += divGx + divGy;
        }
    }
}

/* calculate the sum of divergences for the all pyramid level. the smaller
 * divergence map is upsamled and added to the divergence map for the higher
 * level of pyramid.
 * temp is a temporary matrix of size (cols, rows), assumed to already be
 * allocated
 */
static void
mantiuk06_pyramid_calculate_divergence_sum (pyramid_t *pyramid,
                                            gfloat    *divG_sum)
{
  gfloat *temp = mantiuk06_matrix_alloc (pyramid->rows*pyramid->cols);

  /* Find the coarsest pyramid, and the number of pyramid levels */
  int levels = 1;
  while (pyramid->next != NULL)
    {
      levels++;
      pyramid = pyramid->next;
    }

  /* For every level, we swap temp and divG_sum.  So, if there are an odd
   * number of levels...
   */
  if (levels % 2)
    {
      gfloat *const dummy = divG_sum;
      divG_sum = temp;
      temp = dummy;
    }

  /* Add them all together */
  while (pyramid != NULL)
    {
      gfloat *dummy;

      /* Upsample or zero as needed */
      if (pyramid->next != NULL)
        {
          mantiuk06_matrix_upsample (pyramid->cols,
                                     pyramid->rows,
                                     divG_sum,
                                     temp);
        }
      else
        {
          mantiuk06_matrix_zero (pyramid->rows * pyramid->cols, temp);
        }

      /* Add in the (freshly calculated) divergences */
      mantiuk06_calculate_and_add_divergence (pyramid->cols,
                                              pyramid->rows,
                                              pyramid->Gx,
                                              pyramid->Gy,
                                              temp);

      /* Rather than copying, just switch round the pointers: we know we get
       * them the right way round at the end.
       */
      dummy    = divG_sum;
      divG_sum = temp;
      temp     = dummy;

      pyramid  = pyramid->prev;
    }

  mantiuk06_matrix_free (temp);
}

/* calculate scale factors (Cx,Cy) for gradients (Gx,Gy)
 * C is equal to EDGE_WEIGHT for gradients smaller than GFIXATE or
 * 1.0 otherwise
 */
static inline void
mantiuk06_calculate_scale_factor (const gint          n,
                                  const gfloat *const G,
                                  gfloat       *const C)
{
  const gfloat detectT = 0.001f;
  const gfloat a = 0.038737;
  const gfloat b = 0.537756;

  gint i;

  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
    {
#if 1
      const gfloat g = MAX (detectT, fabsf (G[i]));
      C[i] = 1.0f / (a * powf (g, b));
#else
      const gfloat GFIXATE     = 0.10f,
                  EDGE_WEIGHT = 0.01f;

      if (fabsf (G[i]) < GFIXATE)
        C[i] = 1.0f / EDGE_WEIGHT;
      else
        C[i] = 1.0f;
#endif
    }
}

/* calculate scale factor for the whole pyramid */
static void
mantiuk06_pyramid_calculate_scale_factor (pyramid_t *pyramid,
                                          pyramid_t *pC)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      mantiuk06_calculate_scale_factor (size, pyramid->Gx, pC->Gx);
      mantiuk06_calculate_scale_factor (size, pyramid->Gy, pC->Gy);
      pyramid = pyramid->next;
      pC = pC->next;
    }
}

/* Scale gradient (Gx and Gy) by C (Cx and Cy)
 * G = G / C
 */
static inline void
mantiuk06_scale_gradient (const gint          n,
                          gfloat       *const G,
                          const gfloat *const C)
{
  gint i;
  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
    G[i] *= C[i];
}

/* scale gradients for the whole one pyramid with the use of (Cx,Cy) from the
 * other pyramid
 */
static void
mantiuk06_pyramid_scale_gradient (pyramid_t *pyramid,
                                  pyramid_t *pC)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      mantiuk06_scale_gradient (size, pyramid->Gx, pC->Gx);
      mantiuk06_scale_gradient (size, pyramid->Gy, pC->Gy);
      pyramid = pyramid->next;
      pC = pC->next;
    }
}


/* free memory allocated for the pyramid */
static void
mantiuk06_pyramid_free (pyramid_t *pyramid)
{
  while (pyramid)
    {
      pyramid_t *const next = pyramid->next;

      if (pyramid->Gx != NULL)
        {
          g_free (pyramid->Gx);
          pyramid->Gx = NULL;
        }

      if (pyramid->Gy != NULL)
        {
          g_free (pyramid->Gy);
          pyramid->Gy = NULL;
        }

      pyramid->prev = NULL;
      pyramid->next = NULL;
      g_free (pyramid);
      pyramid = next;
    }
}


/* allocate memory for the pyramid */
static pyramid_t*
mantiuk06_pyramid_allocate (int cols,
                            int rows)
{
  pyramid_t *level = NULL;
  pyramid_t *pyramid = NULL;
  pyramid_t *prev = NULL;

  while (rows >= PYRAMID_MIN_PIXELS && cols >= PYRAMID_MIN_PIXELS)
    {
      guint size;
      level = g_new (pyramid_t, 1);
      memset (level, 0, sizeof (pyramid_t));

      level->rows = rows;
      level->cols = cols;
      size        = level->rows * level->cols;
      level->Gx   = mantiuk06_matrix_alloc (size);
      level->Gy   = mantiuk06_matrix_alloc (size);

      level->prev = prev;
      if (prev != NULL)
        prev->next = level;
      prev = level;

      if (pyramid == NULL)
        pyramid = level;

      rows /= 2;
      cols /= 2;
  }

  return pyramid;
}


/* calculate gradients */
static inline void
mantiuk06_calculate_gradient (const gint          cols,
                              const gint          rows,
                              const gfloat *const lum,
                              gfloat       *const Gx,
                              gfloat       *const Gy)
{
  gint ky, kx;

  _OMP (omp parallel for schedule(static))
  for (ky = 0; ky < rows; ky++)
    {
      for (kx = 0; kx < cols; kx++)
        {
          const gint idx = kx + ky*cols;

          if (kx == (cols - 1))
            Gx[idx] = 0;
          else
            Gx[idx] = lum[idx+1] - lum[idx];

          if (ky == (rows - 1))
            Gy[idx] = 0;
          else
            Gy[idx] = lum[idx + cols] - lum[idx];
        }
    }
}


/* calculate gradients for the pyramid
 * lum_temp gets overwritten!
 */
static void
mantiuk06_pyramid_calculate_gradient (pyramid_t *pyramid,
                                      gfloat    *lum_temp)
{
  gfloat *temp = mantiuk06_matrix_alloc ((pyramid->rows / 2) *
                                         (pyramid->cols / 2));
  gfloat *const temp_saved = temp;

  mantiuk06_calculate_gradient (pyramid->cols,
                                pyramid->rows,
                                lum_temp,
                                pyramid->Gx,
                                pyramid->Gy);

  pyramid = pyramid->next;

  while (pyramid)
    {
      gfloat *dummy;
      mantiuk06_matrix_downsample  (pyramid->prev->cols,
                                    pyramid->prev->rows,
                                    lum_temp,
                                    temp);
      mantiuk06_calculate_gradient (pyramid->cols,
                                    pyramid->rows,
                                    temp,
                                    pyramid->Gx,
                                    pyramid->Gy);

      dummy    = lum_temp;
      lum_temp = temp;
      temp     = dummy;

      pyramid = pyramid->next;
  }

  mantiuk06_matrix_free (temp_saved);
}


/* x = -0.25 * b */
static inline void
mantiuk06_solveX (const gint          n,
                  const gfloat *const b,
                  gfloat       *const x)
{
  gint i;

  _OMP (omp parallel for schedule(static))
  for (i = 0; i < n; i++)
    x[i] = -0.25f * b[i];
}

/* divG_sum = A * x = sum (divG (x))
 * memory for the temporary pyramid px should be allocated
 */
static inline void
mantiuk06_multiplyA (pyramid_t           *px,
                     pyramid_t           *pC,
                     const gfloat *const  x,
                     gfloat       *const  divG_sum)
{
  /* use divG_sum as a temp variable */
  mantiuk06_matrix_copy (pC->rows*pC->cols, x, divG_sum);
  mantiuk06_pyramid_calculate_gradient (px, divG_sum);
  /* scale gradients by Cx,Cy from main pyramid */
  mantiuk06_pyramid_scale_gradient (px, pC);
  /* calculate the sum of divergences */
  mantiuk06_pyramid_calculate_divergence_sum (px, divG_sum);
}


/* bi-conjugate linear equation solver
 * overwrites pyramid!
 */
static void
mantiuk06_linbcg (pyramid_t           *pyramid,
                  pyramid_t           *pC,
                  const gfloat *const  b,
                  gfloat       *const  x,
                  const gint           itmax,
                  const gfloat         tol,
                  pfstmo_progress_callback progress_cb)
{
  const guint  rows = pyramid->rows,
               cols = pyramid->cols,
               n    = rows*cols;
  const gfloat tol2 = tol*tol;

  gfloat *const z      = mantiuk06_matrix_alloc (n),
         *const zz     = mantiuk06_matrix_alloc (n),
         *const p      = mantiuk06_matrix_alloc (n),
         *const pp     = mantiuk06_matrix_alloc (n),
         *const r      = mantiuk06_matrix_alloc (n),
         *const rr     = mantiuk06_matrix_alloc (n),
         *const x_save = mantiuk06_matrix_alloc (n);

  const gfloat bnrm2 = mantiuk06_matrix_dot_product (n, b, b);

  gfloat      err2, bkden, saved_err2, ierr2, percent_sf;
  gint        iter  = 0, num_backwards = 0, num_backwards_ceiling = 3;
  gboolean    reset = TRUE;

  mantiuk06_multiplyA (pyramid, pC, x, r); /* r = A*x = divergence (x) */
  mantiuk06_matrix_subtract (n, b, r);     /* r = b - r               */
  err2 = mantiuk06_matrix_dot_product (n, r, r); /* err2 = r.r */

  mantiuk06_multiplyA (pyramid, pC, r, rr); /* rr = A*r */

  bkden = 0;
  saved_err2 = err2;
  mantiuk06_matrix_copy (n, x, x_save);

  ierr2 = err2;
  percent_sf = 100.0f /logf (tol2 * bnrm2 / ierr2);

  for (; iter < itmax; iter++)
    {
      guint i;
      gfloat bknum, ak, old_err2;

      if (progress_cb != NULL)
        progress_cb ((int) (logf (err2 / ierr2) * percent_sf));

      mantiuk06_solveX (n,  r,  z); /*  z = ~A (-1) *  r = -0.25 *  r */
      mantiuk06_solveX (n, rr, zz); /* zz = ~A (-1) * rr = -0.25 * rr */

      bknum = mantiuk06_matrix_dot_product (n, z, rr);

      if (reset)
        {
          reset = FALSE;
          mantiuk06_matrix_copy (n, z, p);
          mantiuk06_matrix_copy (n, zz, pp);
        }
      else
        {
          const gfloat bk = bknum / bkden; /* beta = ...  */

          _OMP (omp parallel for schedule(static))
          for (i = 0; i < n; i++)
            {
              p[i]  =  z[i] + bk *  p[i];
              pp[i] = zz[i] + bk * pp[i];
            }
        }

      bkden = bknum; /* numerator becomes the dominator for the next iteration */

      mantiuk06_multiplyA (pyramid, pC,  p,  z); /*  z = A* p = divergence (p) */
      mantiuk06_multiplyA (pyramid, pC, pp, zz); /* zz = A*pp = divergence (pp) */

      ak = bknum / mantiuk06_matrix_dot_product (n, z, pp); /* alfa = ...   */

      _OMP (omp parallel for schedule(static))
      for (i = 0 ; i < n ; i++ )
        {
          r[i]  -= ak *  z[i];  /*  r =  r - alfa *  z  */
          rr[i] -= ak * zz[i];  /* rr = rr - alfa * zz  */
        }

      old_err2 = err2;
      err2 = mantiuk06_matrix_dot_product (n, r, r);

      /* Have we gone unstable? */
      if (err2 > old_err2)
        {
          /* Save where we've got to if it's the best yet */
          if (num_backwards == 0 && old_err2 < saved_err2)
            {
              saved_err2 = old_err2;
              mantiuk06_matrix_copy (n, x, x_save);
            }

          num_backwards++;
        }
      else
        {
          num_backwards = 0;
        }

      _OMP (omp parallel for schedule(static))
      for (i = 0 ; i < n ; i++ )
        x[i] += ak * p[i];      /* x =  x + alfa * p */

      if (num_backwards > num_backwards_ceiling)
        {
          /* Reset  */
          reset = TRUE;
          num_backwards = 0;

          /* Recover saved value */
          mantiuk06_matrix_copy (n, x_save, x);

          /* r = Ax */
          mantiuk06_multiplyA (pyramid, pC, x, r);

          /* r = b - r */
          mantiuk06_matrix_subtract (n, b, r);

          /* err2 = r.r  */
          err2 = mantiuk06_matrix_dot_product (n, r, r);
          saved_err2 = err2;

          /* rr = A*r  */
          mantiuk06_multiplyA (pyramid, pC, r, rr);
        }

      if (err2 / bnrm2 < tol2)
        break;
    }

  /* Use the best version we found */
  if (err2 > saved_err2)
    {
      err2 = saved_err2;
      mantiuk06_matrix_copy (n, x_save, x);
    }

  if (err2/bnrm2 > tol2)
    {
      /* Not converged */
      if (progress_cb != NULL)
        progress_cb ((int) (logf (err2 / ierr2) * percent_sf));
      if (iter == itmax)
        g_warning ("mantiuk06: Warning: "
                   "Not converged (hit maximum iterations), "
                   "error = %g (should be below %g).",
                   sqrtf (err2 / bnrm2), tol);
      else
        g_warning ("mantiuk06: Warning: "
                   "Not converged (going unstable), "
                   "error = %g (should be below %g).",
                   sqrtf (err2 / bnrm2), tol);
    }
  else if (progress_cb != NULL)
    progress_cb (100);

  mantiuk06_matrix_free (x_save);
  mantiuk06_matrix_free (p);
  mantiuk06_matrix_free (pp);
  mantiuk06_matrix_free (z);
  mantiuk06_matrix_free (zz);
  mantiuk06_matrix_free (r);
  mantiuk06_matrix_free (rr);
}


/* conjugate linear equation solver
 * overwrites pyramid!
 */
static void
mantiuk06_lincg (pyramid_t           *pyramid,
                 pyramid_t           *pC,
                 const gfloat *const  b,
                 gfloat       *const  x,
                 const int            itmax,
                 const gfloat         tol,
                 pfstmo_progress_callback progress_cb)
{
  const int rows = pyramid->rows,
            cols = pyramid->cols,
            n    = rows*cols;
  int       iter = 0, num_backwards = 0, num_backwards_ceiling = 3;
  const gfloat tol2 = tol*tol;

  gfloat *const x_save = mantiuk06_matrix_alloc (n),
         *const r      = mantiuk06_matrix_alloc (n),
         *const p      = mantiuk06_matrix_alloc (n),
         *const Ap     = mantiuk06_matrix_alloc (n);

  /* bnrm2 = ||b|| */
  const gfloat bnrm2 = mantiuk06_matrix_dot_product (n, b, b);
  gfloat       rdotr, irdotr, saved_rdotr, percent_sf;

  /* r = b - Ax */
  mantiuk06_multiplyA (pyramid, pC, x, r);
  mantiuk06_matrix_subtract (n, b, r);
  rdotr = mantiuk06_matrix_dot_product (n, r, r); /* rdotr = r.r */

  /* p = r */
  mantiuk06_matrix_copy (n, r, p);

  /* Setup initial vector */
  saved_rdotr = rdotr;
  mantiuk06_matrix_copy (n, x, x_save);

  irdotr = rdotr;
  percent_sf = 100.0f / logf (tol2 * bnrm2 / irdotr);
  for (; iter < itmax; iter++)
    {
      gint  i;
      gfloat alpha, old_rdotr;

      if (progress_cb != NULL) {
        gint ret = progress_cb ((gint) (logf (rdotr / irdotr) * percent_sf));
        if (ret == PFSTMO_CB_ABORT && iter > 0 ) /* User requested abort */
          break;
      }

      /* Ap = A p */
      mantiuk06_multiplyA (pyramid, pC, p, Ap);

      /* alpha = r.r / (p . Ap) */
      alpha = rdotr / mantiuk06_matrix_dot_product (n, p, Ap);

      /* r = r - alpha Ap */
      _OMP (omp parallel for schedule(static))
      for (i = 0; i < n; i++)
        r[i] -= alpha * Ap[i];

      /* rdotr = r.r */
      old_rdotr = rdotr;
      rdotr = mantiuk06_matrix_dot_product (n, r, r);

      /* Have we gone unstable? */
      if (rdotr > old_rdotr)
        {
          /* Save where we've got to */
          if (num_backwards == 0 && old_rdotr < saved_rdotr)
            {
              saved_rdotr = old_rdotr;
              mantiuk06_matrix_copy (n, x, x_save);
            }

          num_backwards++;
        }
      else
        {
          num_backwards = 0;
        }

      /* x = x + alpha p */
      _OMP (omp parallel for schedule(static))
      for (i = 0; i < n; i++)
        x[i] += alpha * p[i];


      /* Exit if we're done */
      if (rdotr/bnrm2 < tol2)
        break;

      if (num_backwards > num_backwards_ceiling)
        {
          /* Reset */
          num_backwards = 0;
          mantiuk06_matrix_copy (n, x_save, x);

          /* r = Ax */
          mantiuk06_multiplyA (pyramid, pC, x, r);

          /* r = b - r */
          mantiuk06_matrix_subtract (n, b, r);

          /* rdotr = r.r */
          rdotr = mantiuk06_matrix_dot_product (n, r, r);
          saved_rdotr = rdotr;

          /* p = r */
          mantiuk06_matrix_copy (n, r, p);
        }
      else
        {
          /* p = r + beta p */
          const gfloat beta = rdotr/old_rdotr;

          _OMP (omp parallel for schedule(static))
          for (i = 0; i < n; i++)
            p[i] = r[i] + beta*p[i];
        }
    }

  /* Use the best version we found */
  if (rdotr > saved_rdotr)
    {
      rdotr = saved_rdotr;
      mantiuk06_matrix_copy (n, x_save, x);
    }

  if (rdotr/bnrm2 > tol2)
    {
      /* Not converged */
      if (progress_cb != NULL)
        progress_cb ((int) (logf (rdotr / irdotr) * percent_sf));
      if (iter == itmax)
        g_warning ("mantiuk06: Warning: "
                   "Not converged (hit maximum iterations), "
                   "error = %g (should be below %g).",
                   sqrtf (rdotr/bnrm2), tol);
      else
        g_warning ("mantiuk06: Warning: "
                   "Not converged (going unstable), "
                   "error = %g (should be below %g).",
                   sqrtf (rdotr/bnrm2), tol);
    }
  else if (progress_cb != NULL)
    progress_cb (100);

  mantiuk06_matrix_free (x_save);
  mantiuk06_matrix_free (p);
  mantiuk06_matrix_free (Ap);
  mantiuk06_matrix_free (r);
}


/* in_tab and out_tab should contain inccreasing float values */
static inline gfloat
mantiuk06_lookup_table (const gint          n,
                        const gfloat *const in_tab,
                        const gfloat *const out_tab,
                        const gfloat        val)
{
  gint j;

  if (G_UNLIKELY (val < in_tab[0]))
    return out_tab[0];

  for (j = 1; j < n; j++)
    if (val < in_tab[j])
      {
        const gfloat dd = (val - in_tab[j-1]) / (in_tab[j] - in_tab[j-1]);
        return out_tab[j-1] + (out_tab[j] - out_tab[j-1]) * dd;
      }

  return out_tab[n-1];
}


/* transform gradient (Gx,Gy) to R */
static inline void
mantiuk06_transform_to_R (const gint        n,
                          gfloat     *const G)
{
  gint j;

  _OMP (omp parallel for schedule(static))
  for (j = 0; j < n; j++)
    {
      /* G to W */
      const gfloat absG = fabsf (G[j]);
      int sign;
      if (G[j] < 0)
        sign = -1;
      else
        sign = 1;
      G[j] = (powf (10, absG) - 1.0f) * sign;

      /* W to RESP */
      if (G[j] < 0)
        sign = -1;
      else
        sign = 1;

      G[j] = sign * mantiuk06_lookup_table (LOOKUP_W_TO_R,
                                            W_table,
                                            R_table,
                                            fabsf (G[j]));
    }
}

/* transform gradient (Gx,Gy) to R for the whole pyramid */
static inline void
mantiuk06_pyramid_transform_to_R (pyramid_t *pyramid)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      mantiuk06_transform_to_R (size, pyramid->Gx);
      mantiuk06_transform_to_R (size, pyramid->Gy);
      pyramid = pyramid->next;
    }
}

/* transform from R to G */
static inline void
mantiuk06_transform_to_G (const gint        n,
                          gfloat     *const R)
{
  gint j;

  _OMP (omp parallel for schedule(static))
  for (j = 0; j < n; j++){
    /* RESP to W */
    gint sign;
    if (R[j] < 0)
      sign = -1;
    else
      sign = 1;
    R[j] = sign * mantiuk06_lookup_table (LOOKUP_W_TO_R,
                                          R_table,
                                          W_table,
                                          fabsf (R[j]));

    /* W to G */
    if (R[j] < 0)
      sign = -1;
    else
      sign = 1;
    R[j] = log10f (fabsf (R[j]) + 1.0f) * sign;

  }
}

/* transform from R to G for the pyramid */
static inline void
mantiuk06_pyramid_transform_to_G (pyramid_t *pyramid)
{
  while (pyramid != NULL)
    {
      mantiuk06_transform_to_G (pyramid->rows*pyramid->cols, pyramid->Gx);
      mantiuk06_transform_to_G (pyramid->rows*pyramid->cols, pyramid->Gy);
      pyramid = pyramid->next;
    }
}

/* multiply gradient (Gx,Gy) values by float scalar value for the
 * whole pyramid
 */
static inline void
mantiuk06_pyramid_gradient_multiply (pyramid_t    *pyramid,
                                     const gfloat  val)
{
  while (pyramid != NULL)
    {
      mantiuk06_matrix_multiply_const (pyramid->rows * pyramid->cols,
                                       pyramid->Gx, val);
      mantiuk06_matrix_multiply_const (pyramid->rows * pyramid->cols,
                                       pyramid->Gy, val);
      pyramid = pyramid->next;
    }
}


static int
mantiuk06_sort_float (const void *const v1,
                      const void *const v2)
{
  if (*((gfloat*)v1) < *((gfloat*)v2))
    return -1;

  if (G_LIKELY (*((gfloat*)v1) > *((gfloat*)v2)))
    return 1;

  return 0;
}


/* transform gradients to luminance */
static void
mantiuk06_transform_to_luminance (pyramid_t                        *pp,
                                  gfloat                    *const  x,
                                  pfstmo_progress_callback         progress,
                                  const gboolean                   bcg,
                                  const gint                       itmax,
                                  const gfloat                     tol)
{
  gfloat     *b;
  pyramid_t  *pC = mantiuk06_pyramid_allocate (pp->cols, pp->rows);
  /* calculate (Cx,Cy) */
  mantiuk06_pyramid_calculate_scale_factor (pp, pC);
  /* scale small gradients by (Cx,Cy); */
  mantiuk06_pyramid_scale_gradient (pp, pC);

  b = mantiuk06_matrix_alloc (pp->cols * pp->rows);
  /* calculate the sum of divergences (equal to b) */
  mantiuk06_pyramid_calculate_divergence_sum (pp, b);

  /* calculate luminances from gradients */
  if (bcg)
    mantiuk06_linbcg (pp, pC, b, x, itmax, tol, progress);
  else
    mantiuk06_lincg (pp, pC, b, x, itmax, tol, progress);

  mantiuk06_matrix_free (b);
  mantiuk06_pyramid_free (pC);
}


struct hist_data
{
  gfloat size;
  gfloat cdf;
  gint index;
};

static int
mantiuk06_hist_data_order (const void *const v1,
                           const void *const v2)
{
  if (((struct hist_data*) v1)->size < ((struct hist_data*) v2)->size)
    return -1;

  if (((struct hist_data*) v1)->size > ((struct hist_data*) v2)->size)
    return 1;

  return 0;
}


static int
mantiuk06_hist_data_index (const void *const v1,
                           const void *const v2)
{
  return ((struct hist_data*) v1)->index - ((struct hist_data*) v2)->index;
}


static void
mantiuk06_contrast_equalization (pyramid_t   *pp,
                                 const gfloat  contrastFactor )
{
  gint              i, idx;
  struct hist_data *hist;
  gint              total_pixels = 0;

  /* Count sizes */
  pyramid_t *l = pp;
  while (l != NULL)
    {
      total_pixels += l->rows * l->cols;
      l = l->next;
    }

  /* Allocate memory */
  hist = g_new (struct hist_data, total_pixels);

  /* Build histogram info */
  l   = pp;
  idx = 0;
  while (l != NULL)
    {
      const int pixels = l->rows*l->cols;
      const int offset = idx;
      gint      c;

      _OMP (omp parallel for schedule(static))
      for (c = 0; c < pixels; c++)
        {
          hist[c+offset].size = sqrtf (l->Gx[c] * l->Gx[c] +
                                       l->Gy[c] * l->Gy[c]);
          hist[c+offset].index = c + offset;
        }
      idx += pixels;
      l = l->next;
    }

  /* Generate histogram */
  qsort (hist, total_pixels, sizeof (struct hist_data),
         mantiuk06_hist_data_order);

  /* Calculate cdf */
  {
    const gfloat norm = 1.0f / (gfloat) total_pixels;
    _OMP (omp parallel for schedule(static))
    for (i = 0; i < total_pixels; i++)
      hist[i].cdf = ((gfloat) i) * norm;
  }

  /* Recalculate in terms of indexes */
  qsort (hist, total_pixels,
         sizeof (struct hist_data),
         mantiuk06_hist_data_index);

  /*Remap gradient magnitudes */
  l   = pp;
  idx = 0;
  while (l != NULL )
    {
      gint      c;
      const int pixels = l->rows*l->cols;
      const int offset = idx;

      _OMP (omp parallel for schedule(static))
      for (c = 0; c < pixels; c++)
        {
          const gfloat scale = contrastFactor      *
                               hist[c+offset].cdf  /
                               hist[c+offset].size;
          l->Gx[c] *= scale;
          l->Gy[c] *= scale;
        }
      idx += pixels;
      l    = l->next;
    }

  g_free (hist);
}


/* tone mapping */
static int
mantiuk06_contmap (const int                       c,
                   const int                       r,
                   gfloat                   *const rgb,
                   gfloat                   *const Y,
                   const gfloat                    contrastFactor,
                   const gfloat                    saturationFactor,
                   const gboolean                  bcg,
                   const int                       itmax,
                   const gfloat                    tol,
                   pfstmo_progress_callback        progress)
{
  const guint n = c*r;
        guint j;

  /* Normalize */
  gfloat Ymax = Y[0],
         clip_min;

  for (j = 1; j < n; j++)
      Ymax = MAX (Y[j], Ymax);

  clip_min = 1e-7f * Ymax;
  _OMP (omp parallel for schedule(static))
  for (j = 0; j < n * 4; j++)
      if (G_UNLIKELY (rgb[j] < clip_min)) rgb[j] = clip_min;

  _OMP (omp parallel for schedule(static))
  for (j = 0; j < n; j++)
      if (G_UNLIKELY (  Y[j] < clip_min))   Y[j] = clip_min;

  _OMP (omp parallel for schedule(static))
  for (j = 0; j < n; j++)
    {
      rgb[j * 4 + 0] /= Y[j];
      rgb[j * 4 + 1] /= Y[j];
      rgb[j * 4 + 2] /= Y[j];
      Y[j]            = log10f (Y[j]);
    }

  {
    /* create pyramid */
    pyramid_t *pp = mantiuk06_pyramid_allocate (c,r);
    gfloat    *tY = mantiuk06_matrix_alloc (n);

    /* copy Y to tY */
    mantiuk06_matrix_copy (n, Y, tY);
    /* calculate gradients for pyramid, destroys tY */
    mantiuk06_pyramid_calculate_gradient (pp,tY);
    mantiuk06_matrix_free (tY);
    /* transform gradients to R */
    mantiuk06_pyramid_transform_to_R (pp);

    /* Contrast map */
    if (contrastFactor > 0.0f)
      /* Contrast mapping */
      mantiuk06_pyramid_gradient_multiply (pp, contrastFactor);
    else
      /* Contrast equalization */
      mantiuk06_contrast_equalization (pp, -contrastFactor);

    /* transform R to gradients */
    mantiuk06_pyramid_transform_to_G (pp);
    /* transform gradients to luminance Y */
    mantiuk06_transform_to_luminance (pp, Y, progress, bcg, itmax, tol);
    mantiuk06_pyramid_free (pp);
  }

  /* Renormalize luminance */
  {
    const gfloat CUT_MARGIN = 0.1f;
    gfloat      *temp = mantiuk06_matrix_alloc (n),
                 trim, delta, l_min, l_max;

    /* copy Y to temp */
    mantiuk06_matrix_copy (n, Y, temp);
    /* sort temp in ascending order */
    qsort (temp, n, sizeof (gfloat), mantiuk06_sort_float);

    /* const float median = (temp[(int)((n-1)/2)] + temp[(int)((n-1)/2+1)]) * 0.5f; */
    /* calculate median */
    trim  = (n - 1) * CUT_MARGIN * 0.01f;
    delta = trim - floorf (trim);
    l_min = temp[(int)floorf (trim)] * delta +
            temp[(int) ceilf (trim)] * (1.0f - delta);

    trim  = (n - 1) * (100.0f - CUT_MARGIN) * 0.01f;
    delta = trim - floorf (trim);
    l_max = temp[(int)floorf (trim)] * delta +
            temp[(int) ceilf (trim)] * (1.0f - delta);

    mantiuk06_matrix_free (temp);
    {
      const gfloat disp_dyn_range = 2.3f;
      _OMP (omp parallel for schedule(static))
      for (j = 0; j < n; j++)
          /* x scaled */
          Y[j] = ( Y[j] - l_min) /
                 (l_max - l_min) *
                 disp_dyn_range - disp_dyn_range;
    }

    /* Transform to linear scale RGB */
    _OMP (omp parallel for schedule(static))
    for (j = 0; j < n; j++)
      {
        Y[j] = powf (10,Y[j]);

        rgb[j * 4 + 0] = powf (rgb[j * 4 + 0], saturationFactor) * Y[j];
        rgb[j * 4 + 1] = powf (rgb[j * 4 + 1], saturationFactor) * Y[j];
        rgb[j * 4 + 2] = powf (rgb[j * 4 + 2], saturationFactor) * Y[j];
      }
  }

  return PFSTMO_OK;
}


static void
mantiuk06_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format (OUTPUT_FORMAT));
  gegl_operation_set_format (operation, "output", babl_format (OUTPUT_FORMAT));
}


static GeglRectangle
mantiuk06_get_required_for_output (GeglOperation        *operation,
                                    const gchar         *input_pad,
                                    const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}


static GeglRectangle
mantiuk06_get_cached_region (GeglOperation        *operation,
                              const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}


static gboolean
mantiuk06_process (GeglOperation       *operation,
                   GeglBuffer          *input,
                   GeglBuffer          *output,
                   const GeglRectangle *result,
                   gint                 level)
{
  const GeglChantO *o          = GEGL_CHANT_PROPERTIES (operation);
  const gint        pix_stride = 4; /* RGBA */
  gfloat           *lum,
                   *pix;

  g_return_val_if_fail (operation, FALSE);
  g_return_val_if_fail (input, FALSE);
  g_return_val_if_fail (output, FALSE);
  g_return_val_if_fail (result, FALSE);

  g_return_val_if_fail (babl_format_get_n_components (babl_format (OUTPUT_FORMAT)) == pix_stride, FALSE);

  /* Obtain the pixel data */
  lum = g_new (gfloat, result->width * result->height),
  gegl_buffer_get (input, result, 1.0, babl_format ("Y float"),
                   lum, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  pix = g_new (gfloat, result->width * result->height * pix_stride);
  gegl_buffer_get (input, result, 1.0, babl_format (OUTPUT_FORMAT),
                   pix, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  mantiuk06_contmap (result->width, result->height, pix, lum,
                     o->contrast, o->saturation, FALSE, 200, 1e-3, NULL);

  /* Cleanup and set the output */
  gegl_buffer_set (output, result, 0, babl_format (OUTPUT_FORMAT), pix,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (pix);
  g_free (lum);

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

  filter_class->process = mantiuk06_process;

  operation_class->prepare                 = mantiuk06_prepare;
  operation_class->get_required_for_output = mantiuk06_get_required_for_output;
  operation_class->get_cached_region       = mantiuk06_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
      "name"       , "gegl:mantiuk06",
      "categories" , "tonemapping",
      "description",
        _("Adapt an image, which may have a high dynamic range, for "
          "presentation using a low dynamic range. This operator constrains "
          "contrasts across multiple spatial frequencies, producing "
          "luminance within the range 0.0-1.0"),
        NULL);
}

#endif

