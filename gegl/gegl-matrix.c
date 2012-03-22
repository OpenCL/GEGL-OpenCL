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
static void gegl_matrix3_debug (GeglMatrix3 *matrix)
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

GeglMatrix3 *
gegl_matrix3_new (void)
{
  return g_new0(GeglMatrix3, 1);
}

GType
gegl_matrix3_get_type (void)
{
  static GType matrix_type = 0;

  if (!matrix_type) {
    matrix_type = g_boxed_type_register_static ("GeglMatrix3",
                                               (GBoxedCopyFunc) gegl_matrix3_copy,
                                               (GBoxedFreeFunc) g_free);
  }

  return matrix_type;
}

void
gegl_matrix3_identity (GeglMatrix3 *matrix)
{
  matrix->coeff [0][0] = matrix->coeff [1][1] = matrix->coeff [2][2] = 1.;

  matrix->coeff [0][1] = matrix->coeff [0][2] = 0.;
  matrix->coeff [1][0] = matrix->coeff [1][2] = 0.;
  matrix->coeff [2][0] = matrix->coeff [2][1] = 0.;
}

gboolean
gegl_matrix3_equal (GeglMatrix3 *matrix1,
                    GeglMatrix3 *matrix2)
{
  gint x, y;

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      if (matrix1->coeff [y][x] != matrix2->coeff [y][x])
        return FALSE;
  return TRUE;
}

gboolean
gegl_matrix3_is_identity (GeglMatrix3 *matrix)
{
  GeglMatrix3 identity;
  gegl_matrix3_identity (&identity);
  return gegl_matrix3_equal (&identity, matrix);
}

gboolean
gegl_matrix3_is_scale (GeglMatrix3 *matrix)
{
  GeglMatrix3 copy;
  gegl_matrix3_copy_into (&copy, matrix);
  copy.coeff [0][0] = copy.coeff [1][1] = 1.;
  copy.coeff [0][2] = copy.coeff [1][2] = 0.;
  return gegl_matrix3_is_identity (&copy);
}

gboolean
gegl_matrix3_is_translate (GeglMatrix3 *matrix)
{
  GeglMatrix3 copy;
  gegl_matrix3_copy_into (&copy, matrix);
  copy.coeff [0][2] = copy.coeff [1][2] = 0.;
  return gegl_matrix3_is_identity (&copy);
}

void
gegl_matrix3_copy_into (GeglMatrix3 *dst,
                        GeglMatrix3 *src)
{
  memcpy (dst->coeff [0], src->coeff [0], 3 * sizeof (gdouble));
  memcpy (dst->coeff [1], src->coeff [1], 3 * sizeof (gdouble));
  memcpy (dst->coeff [2], src->coeff [2], 3 * sizeof (gdouble));
}

GeglMatrix3 *
gegl_matrix3_copy (GeglMatrix3 *matrix)
{
  return (GeglMatrix3 *) g_memdup (matrix, sizeof (GeglMatrix3));
}

gdouble
gegl_matrix3_determinant (GeglMatrix3 *matrix)
{
  gdouble determinant;

  determinant = matrix->coeff [0][0] * (matrix->coeff [1][1] * matrix->coeff [2][2] -
                                 matrix->coeff [1][2] * matrix->coeff [2][1])
              - matrix->coeff [0][1] * (matrix->coeff [1][0] * matrix->coeff [2][2] -
                                 matrix->coeff [1][2] * matrix->coeff [2][0])
              + matrix->coeff [0][2] * (matrix->coeff [1][0] * matrix->coeff [2][1] -
                                 matrix->coeff [1][1] * matrix->coeff [2][0]);
  return determinant;
}

void
gegl_matrix3_invert (GeglMatrix3 *matrix)
{
  GeglMatrix3 copy;
  gdouble coeff;

  gegl_matrix3_copy_into (&copy, matrix);
  coeff = 1 / gegl_matrix3_determinant (matrix);

  matrix->coeff [0][0] = (copy.coeff [1][1] * copy.coeff [2][2] -
                   copy.coeff [1][2] * copy.coeff [2][1]) * coeff;
  matrix->coeff [1][0] = (copy.coeff [1][2] * copy.coeff [2][0] -
                   copy.coeff [1][0] * copy.coeff [2][2]) * coeff;
  matrix->coeff [2][0] = (copy.coeff [1][0] * copy.coeff [2][1] -
                   copy.coeff [1][1] * copy.coeff [2][0]) * coeff;

  matrix->coeff [0][1] = (copy.coeff [0][2] * copy.coeff [2][1] -
                   copy.coeff [0][1] * copy.coeff [2][2]) * coeff;
  matrix->coeff [1][1] = (copy.coeff [0][0] * copy.coeff [2][2] -
                   copy.coeff [0][2] * copy.coeff [2][0]) * coeff;
  matrix->coeff [2][1] = (copy.coeff [0][1] * copy.coeff [2][0] -
                   copy.coeff [0][0] * copy.coeff [2][1]) * coeff;

  matrix->coeff [0][2] = (copy.coeff [0][1] * copy.coeff [1][2] -
                   copy.coeff [0][2] * copy.coeff [1][1]) * coeff;
  matrix->coeff [1][2] = (copy.coeff [0][2] * copy.coeff [1][0] -
                   copy.coeff [0][0] * copy.coeff [1][2]) * coeff;
  matrix->coeff [2][2] = (copy.coeff [0][0] * copy.coeff [1][1] -
                   copy.coeff [0][1] * copy.coeff [1][0]) * coeff;
}


void
gegl_matrix3_multiply (GeglMatrix3 *left,
                       GeglMatrix3 *right,
                       GeglMatrix3 *product)
{
  GeglMatrix3 temp;
  gint    i;

  for (i = 0; i < 3; i++)
    {
      temp.coeff [i][0] = left->coeff [i][0] * right->coeff [0][0]
                    + left->coeff [i][1] * right->coeff [1][0]
                    + left->coeff [i][2] * right->coeff [2][0];
      temp.coeff [i][1] = left->coeff [i][0] * right->coeff [0][1]
                    + left->coeff [i][1] * right->coeff [1][1]
                    + left->coeff [i][2] * right->coeff [2][1];
      temp.coeff [i][2] = left->coeff [i][0] * right->coeff [0][2]
                    + left->coeff [i][1] * right->coeff [1][2]
                    + left->coeff [i][2] * right->coeff [2][2];
    }

  gegl_matrix3_copy_into (product, &temp);
}

void
gegl_matrix3_originate (GeglMatrix3 *matrix,
                   gdouble x,
                   gdouble y)
{

  /* assumes last row is [0 0 1] (true for affine transforms) */
  matrix->coeff [0][2] = matrix->coeff [0][0] * (-x) +
                  matrix->coeff [0][1] * (-y) +
                  matrix->coeff [0][2] + x;
  matrix->coeff [1][2] = matrix->coeff [1][0] * (-x) +
                  matrix->coeff [1][1] * (-y) +
                  matrix->coeff [1][2] + y;
}

void
gegl_matrix3_transform_point (GeglMatrix3  *matrix,
                              gdouble     *x,
                              gdouble     *y)
{
  gdouble xp, yp, w;
  
  w = (*x * matrix->coeff [2][0] + *y * matrix->coeff [2][1] + matrix->coeff [2][2]);

  xp = (*x * matrix->coeff [0][0] + *y * matrix->coeff [0][1] + matrix->coeff [0][2]) /w;
  yp = (*x * matrix->coeff [1][0] + *y * matrix->coeff [1][1] + matrix->coeff [1][2]) /w;

  *x = xp;
  *y = yp;
}

void
gegl_matrix3_parse_string (GeglMatrix3  *matrix,
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

      matrix->coeff [0][2] = a;
      matrix->coeff [1][2] = b;
    }
  else if (strstr (string, "matrix"))
    {
      gchar *p = strchr (string, '(');
      gfloat a;
      gint i,j;
      if (!p) return;
      p++;


      for (i=0;i<3;i++)
        for (j=0;j<3;j++)
          {
            a = strtod(p, &p);
            matrix->coeff [j][i] = a;
            if (!p) return;
            p = strchr (p, ',');
            if (!p) return;
            p++;
          }
    }
}

gchar *gegl_matrix3_to_string (GeglMatrix3 *matrix)
{
  gchar *res;
  GString *str = g_string_new ("matrix(");
  gint i, j;
  gint a=0;

  for (i=0;i<3;i++)
    for (j=0;j<3;j++)
      {
        if (a!=0)
          g_string_append (str, ",");
        a=1;
        g_string_append_printf (str, "%f", matrix->coeff[j][i]);
      }
  g_string_append (str, ")");
  res = str->str;
  g_string_free (str, FALSE);

  return res;
}
