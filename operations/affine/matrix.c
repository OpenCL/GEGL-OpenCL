/* This file is part of GEGL
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
 * Copyright 2006 Philip Lafleur
 */

#include <math.h>
#include <string.h>

#include "matrix.h"

void
matrix3_identity (Matrix3 matrix)
{
  matrix [0][0] = matrix [1][1] = matrix [2][2] = 1.;

  matrix [0][1] = matrix [0][2] = 0.;
  matrix [1][0] = matrix [1][2] = 0.;
  matrix [2][0] = matrix [2][1] = 0.;
}

gboolean
matrix3_equal (Matrix3 matrix1,
               Matrix3 matrix2)
{
  gint x, y;

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      if (matrix1 [y][x] != matrix2 [y][x])
        return FALSE;
  return TRUE;
}

gboolean
matrix3_is_identity (Matrix3 matrix)
{
  Matrix3 identity;

  matrix3_identity (identity);
  return matrix3_equal (identity, matrix);
}

gboolean
matrix3_is_scale (Matrix3 matrix)
{
  Matrix3 copy;

  matrix3_copy (copy, matrix);
  copy [0][0] = copy [1][1] = 1.;
  copy [0][2] = copy [1][2] = 0.;
  return matrix3_is_identity (copy);
}

gboolean
matrix3_is_translate (Matrix3 matrix)
{
  Matrix3 copy;

  matrix3_copy (copy, matrix);
  copy [0][2] = copy [1][2] = 0.;
  return matrix3_is_identity (copy);
}

void
matrix3_copy (Matrix3 dst,
              Matrix3 src)
{
  memcpy (dst [0], src [0], 3 * sizeof (gdouble));
  memcpy (dst [1], src [1], 3 * sizeof (gdouble));
  memcpy (dst [2], src [2], 3 * sizeof (gdouble));
}

gdouble
matrix3_determinant (Matrix3 matrix)
{
  gdouble determinant;

  determinant = matrix [0][0] * (matrix [1][1] * matrix [2][2] -
                                 matrix [1][2] * matrix [2][1])
              - matrix [0][1] * (matrix [1][0] * matrix [2][2] -
                                 matrix [1][2] * matrix [2][0])
              + matrix [0][2] * (matrix [1][0] * matrix [2][1] -
                                 matrix [1][1] * matrix [2][0]);
  return determinant;
}

void
matrix3_invert (Matrix3 matrix)
{
  Matrix3 copy;
  gdouble coeff;

  matrix3_copy (copy, matrix);
  coeff = 1 / matrix3_determinant (matrix);

  matrix [0][0] = (copy [1][1] * copy [2][2] -
                   copy [1][2] * copy [2][1]) * coeff;
  matrix [1][0] = (copy [1][2] * copy [2][0] -
                   copy [1][0] * copy [2][2]) * coeff;
  matrix [2][0] = (copy [1][0] * copy [2][1] -
                   copy [1][1] * copy [2][0]) * coeff;

  matrix [0][1] = (copy [0][2] * copy [2][1] -
                   copy [0][1] * copy [2][2]) * coeff;
  matrix [1][1] = (copy [0][0] * copy [2][2] -
                   copy [0][2] * copy [2][0]) * coeff;
  matrix [2][1] = (copy [0][1] * copy [2][0] -
                   copy [0][0] * copy [2][1]) * coeff;

  matrix [0][2] = (copy [0][1] * copy [1][2] -
                   copy [0][2] * copy [1][1]) * coeff;
  matrix [1][2] = (copy [0][2] * copy [1][0] -
                   copy [0][0] * copy [1][2]) * coeff;
  matrix [2][2] = (copy [0][0] * copy [1][1] -
                   copy [0][1] * copy [1][0]) * coeff;
}

void
matrix3_multiply (Matrix3 left,
                  Matrix3 right,
                  Matrix3 product)
{
  Matrix3 temp;
  gint    i;

  for (i = 0; i < 3; i++)
    {
      temp [i][0] = left [i][0] * right [0][0] + left [i][1] * right [1][0]
                    + left [i][2] * right [2][0];
      temp [i][1] = left [i][0] * right [0][1] + left [i][1] * right [1][1]
                    + left [i][2] * right [2][1];
      temp [i][2] = left [i][0] * right [0][2] + left [i][1] * right [1][2]
                    + left [i][2] * right [2][2];
    }

  matrix3_copy (product, temp);
}

void
matrix3_originate (Matrix3 matrix,
                   gdouble x,
                   gdouble y)
{
  /* assumes last row is [0 0 1] (true for affine transforms) */
  matrix [0][2] = matrix [0][0] * (-x) +
                  matrix [0][1] * (-y) +
                  matrix [0][2] + x;
  matrix [1][2] = matrix [1][0] * (-x) +
                  matrix [1][1] * (-y) +
                  matrix [1][2] + y;
}

void
matrix3_transform_point (Matrix3  matrix,
                         gdouble *x,
                         gdouble *y)
{
  gdouble xp,
          yp;

  /* assumes last row is [0 0 1] (true for affine transforms) */
  xp = *x * matrix [0][0] + *y * matrix [0][1] + matrix [0][2];
  yp = *x * matrix [1][0] + *y * matrix [1][1] + matrix [1][2];

  *x = xp;
  *y = yp;
}
