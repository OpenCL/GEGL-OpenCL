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

#ifndef __MATTING_CBLAS_H__
#define __MATTING_CBLAS_H__

enum CBLAS_ORDER
{
  CblasRowMajor = 101,
  CblasColMajor = 102
};


enum CBLAS_TRANSPOSE
{
  CblasNoTrans   = 111,
  CblasTrans     = 112,
  CblasConjTrans = 113
};

   

gint cblas_dgemm(enum CBLAS_ORDER      order,
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
                 gint                  ldc);

#endif
