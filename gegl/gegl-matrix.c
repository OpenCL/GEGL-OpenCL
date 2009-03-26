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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Philip Lafleur
 */

#include "config.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "gegl-matrix.h"


#if 0
static void gegl_matrix3_debug (GeglMatrix3 matrix)
{
  if (matrix)
    {
      gchar *str = gegl_matrix3_to_string (matrix);
      g_print("%s\n", str);
      g_free (str);
    }
  else
    {
      g_print("NULL matrix\n");
    }
}
#endif

void
gegl_matrix3_identity (GeglMatrix3 matrix)
{
  matrix [0][0] = matrix [1][1] = matrix [2][2] = 1.;

  matrix [0][1] = matrix [0][2] = 0.;
  matrix [1][0] = matrix [1][2] = 0.;
  matrix [2][0] = matrix [2][1] = 0.;
}

gboolean
gegl_matrix3_equal (GeglMatrix3 matrix1,
                    GeglMatrix3 matrix2)
{
  gint x, y;

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      if (matrix1 [y][x] != matrix2 [y][x])
        return FALSE;
  return TRUE;
}

gboolean
gegl_matrix3_is_identity (GeglMatrix3 matrix)
{
  GeglMatrix3 identity;

  gegl_matrix3_identity (identity);
  return gegl_matrix3_equal (identity, matrix);
}

gboolean
gegl_matrix3_is_scale (GeglMatrix3 matrix)
{
  GeglMatrix3 copy;

  gegl_matrix3_copy (copy, matrix);
  copy [0][0] = copy [1][1] = 1.;
  copy [0][2] = copy [1][2] = 0.;
  return gegl_matrix3_is_identity (copy);
}

gboolean
gegl_matrix3_is_translate (GeglMatrix3 matrix)
{
  GeglMatrix3 copy;

  gegl_matrix3_copy (copy, matrix);
  copy [0][2] = copy [1][2] = 0.;
  return gegl_matrix3_is_identity (copy);
}

void
gegl_matrix3_copy (GeglMatrix3 dst,
                   GeglMatrix3 src)
{
  memcpy (dst [0], src [0], 3 * sizeof (gdouble));
  memcpy (dst [1], src [1], 3 * sizeof (gdouble));
  memcpy (dst [2], src [2], 3 * sizeof (gdouble));
}

gdouble
gegl_matrix3_determinant (GeglMatrix3 matrix)
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
gegl_matrix3_invert (GeglMatrix3 matrix)
{
  GeglMatrix3 copy;
  gdouble coeff;

  gegl_matrix3_copy (copy, matrix);
  coeff = 1 / gegl_matrix3_determinant (matrix);

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
gegl_matrix3_multiply (GeglMatrix3 left,
                       GeglMatrix3 right,
                       GeglMatrix3 product)
{
  GeglMatrix3 temp;
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

  gegl_matrix3_copy (product, temp);
}

void
gegl_matrix3_originate (GeglMatrix3 matrix,
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
gegl_matrix3_transform_point (GeglMatrix3  matrix,
                              gdouble     *x,
                              gdouble     *y)
{
  gdouble xp,
          yp;

  /* assumes last row is [0 0 1] (true for affine transforms) */
  xp = *x * matrix [0][0] + *y * matrix [0][1] + matrix [0][2];
  yp = *x * matrix [1][0] + *y * matrix [1][1] + matrix [1][2];

  *x = xp;
  *y = yp;
}

void
gegl_matrix3_parse_string (GeglMatrix3  matrix,
                           const gchar *string)
{
  gegl_matrix3_identity (matrix);
  if (strstr (string, "translate"))
    {
      gchar *p = strchr (string, '(');
      gfloat a;
      gfloat b;
      if (!p) return;
      p++;
      a = strtod(p, &p);
      if (!p) return;
      p = strchr (string, ',');
      if (!p) return;
      p++;
      b = strtod (p, &p);
      if (!p) return;

      matrix [0][2] = a;
      matrix [1][2] = b;
    }
  else if (strstr (string, "matrix"))
    {
      gchar *p = strchr (string, '(');
      gfloat a;
      gint i,j;
      if (!p) return;
      p++;


      for (i=0;i<3;i++)
        for (j=0;j<2;j++)
          {
            a = strtod(p, &p);
            matrix [j][i] = a;
            if (!p) return;
            p = strchr (p, ',');
            if (!p) return;
            p++;
          }
    }
}

gchar *gegl_matrix3_to_string (GeglMatrix3 matrix)
{
  gchar *res;
  GString *str = g_string_new ("matrix(");
  gint i, j;
  gint a=0;

  for (i=0;i<3;i++)
    for (j=0;j<2;j++)
      {
        if (a!=0)
          g_string_append (str, ",");
        a=1;
        g_string_append_printf (str, "%f", matrix[j][i]);
      }
  g_string_append (str, ")");
  res = str->str;
  g_string_free (str, FALSE);

  return res;
}
