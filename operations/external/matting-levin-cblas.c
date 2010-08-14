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
 * This is derived from the (public domain) reference implementation on
 * netlib.
 */

#include "config.h"

#include <glib.h>

#include "matting-levin-cblas.h"


/* cblas_dgemm: We include this implementation so that we don't have to
 * depend on cblas solely for this one function. It does not appear to be the
 * performance bottleneck at this time, and the matrices aren't going to be
 * massive.
 *
 * Using restrict may not be correct in the general case, but we can
 * guarantee in our case that the matrices do not overlap.
 */
gint
cblas_dgemm(enum CBLAS_ORDER      order,
            enum CBLAS_TRANSPOSE  transa,
            enum CBLAS_TRANSPOSE  transb,
            gint                  m,
            gint                  n,
            gint                  k,
            gdouble               alpha,
            const gdouble        *restrict a,
            gint                  lda,
            const gdouble        *restrict b,
            gint                  ldb,
            gdouble               beta,
            gdouble              *restrict c,
            gint                  ldc)
{
  /* System generated locals */
  gint i__1, i__2, i__3;

  /* Local variables */
  gint     info;
  gboolean nota, notb;
  gdouble  temp;
  gint     i, j, l, ncola;
  gint     nrowa, nrowb;

  /* Rather than catering for arbitrary indexing, just force row-major */
  //g_return_val_if_fail (order == CblasRowMajor, 1);

#define A(I,J) a[(I)-1 + ((J)-1)* (lda)]
#define B(I,J) b[(I)-1 + ((J)-1)* (ldb)]
#define C(I,J) c[(I)-1 + ((J)-1)* (ldc)]

  if (order == CblasRowMajor)
    return cblas_dgemm (CblasColMajor, transb, transa,
                        n, m, k, alpha, b, ldb, a, lda, beta, c, ldc);

  nota = CblasNoTrans == transa;
  notb = CblasNoTrans == transb;
  if (nota)
    {
      nrowa = m;
      ncola = k;
    }
  else
    {
      nrowa = k;
      ncola = m;
    }
  if (notb)
    {
      nrowb = k;
    }
  else
    {
      nrowb = n;
    }

  /* Test the input parameters. */
  info = 0;
  if (!nota && transa != CblasConjTrans && transa != CblasTrans)
    info = 1;
  else if (!notb && transb != CblasConjTrans && transb != CblasTrans)
    info = 2;
  else if (m < 0)
    info = 3;
  else if (n < 0)
    info = 4;
  else if (k < 0)
    info = 5;
  else if (lda < MAX (1, nrowa))
    info = 8;
  else if (ldb < MAX (1, nrowb))
    info = 10;
  else if (ldc < MAX (1, m))
    info = 13;
  if (info != 0)
    {
      g_warning ("On entry to %s, parameter number %i had an illegal value",
                 "DGEMM ", info);
      return 1;
    }

  /* Start the operations. */
  if (notb)
    {
      if (nota)
        {
          /* Form  C := alpha*A*B + beta*C. */
          i__1 = n;
          for (j = 1; j <= n; ++j)
            {
              if (beta == 0.)
                {
                  i__2 = m;

                  for (i = 1; i <= m; ++i)
                      C(i,j) = 0.;
                }
              else if (beta != 1.)
                {
                  i__2 = m;

                  for (i = 1; i <= m; ++i)
                    C(i,j) = beta * C(i,j);
                }

              i__2 = k;
              for (l = 1; l <= k; ++l)
                {
                  if (B(l,j) != 0.)
                    {
                      temp = alpha * B(l,j);
                      i__3 = m;

                      for (i = 1; i <= m; ++i)
                        C(i,j) += temp * A(i,l);
                    }
                }
            }
        }
      else
        {
          i__1 = n;
          for (j = 1; j <= n; ++j)
            {
              i__2 = m;
              for (i = 1; i <= m; ++i)
                {
                  temp = 0.;
                  i__3 = k;

                  for (l = 1; l <= k; ++l)
                    temp += A(l,i) * B(l,j);

                  if (beta == 0.)
                    C(i,j) = alpha * temp;
                  else
                    C(i,j) = alpha * temp + beta * C(i,j);
                }
            }
        }
    }
  else
    {
      if (nota)
        {
          /* Form  C := alpha*A*B' + beta*C */
          i__1 = n;
          for (j = 1; j <= n; ++j)
            {
              if (beta == 0.)
                {
                  i__2 = m;

                  for (i = 1; i <= m; ++i)
                    C(i,j) = 0.;
                }
              else if (beta != 1.)
                {
                  i__2 = m;

                  for (i = 1; i <= m; ++i)
                    C(i,j) = beta * C(i,j);
                }

              i__2 = k;
              for (l = 1; l <= k; ++l)
                {
                  if (B(j,l) != 0.)
                    {
                      temp = alpha * B(j,l);
                      i__3 = m;

                      for (i = 1; i <= m; ++i)
                        C(i,j) += temp * A(i,l);
                    }
                }
            }
        }
      else
        {
          /* Form  C := alpha*A'*B' + beta*C */

          i__1 = n;
          for (j = 1; j <= n; ++j)
            {
              i__2 = m;
              for (i = 1; i <= m; ++i)
                {
                  temp = 0.;
                  i__3 = k;
                  for (l = 1; l <= k; ++l)
                    temp += A(l,i) * B(j,l);

                  if (beta == 0.)
                    C(i,j) = alpha * temp;
                  else
                    C(i,j) = alpha * temp + beta * C(i,j);
                }
            }
        }
    }

  return 0;
}

