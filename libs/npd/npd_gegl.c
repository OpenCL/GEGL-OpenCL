/*
 * This file is part of N-point image deformation library.
 *
 * N-point image deformation library is free software: you can
 * redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * N-point image deformation library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with N-point image deformation library.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#include "npd_gegl.h"
#include <glib.h>

void
npd_new_matrix (NPDMatrix **matrix)
{
  *matrix = g_new (NPDMatrix, 1);
}

void
npd_destroy_matrix (NPDMatrix **matrix)
{
  g_free (*matrix);
}

void
npd_compute_affinity (NPDPoint  *p11,
                      NPDPoint  *p21,
                      NPDPoint  *p31,
                      NPDPoint  *p12,
                      NPDPoint  *p22,
                      NPDPoint  *p32,
                      NPDMatrix *T)
{
  GeglMatrix3 Y, X;
  
  Y.coeff[0][0] = p12->x; Y.coeff[1][0] = p12->y; Y.coeff[2][0] = 1;
  Y.coeff[0][1] = p22->x; Y.coeff[1][1] = p22->y; Y.coeff[2][1] = 1;
  Y.coeff[0][2] = p32->x; Y.coeff[1][2] = p32->y; Y.coeff[2][2] = 1;
  
  X.coeff[0][0] = p11->x; X.coeff[1][0] = p11->y; X.coeff[2][0] = 1;
  X.coeff[0][1] = p21->x; X.coeff[1][1] = p21->y; X.coeff[2][1] = 1;
  X.coeff[0][2] = p31->x; X.coeff[1][2] = p31->y; X.coeff[2][2] = 1;
  
  gegl_matrix3_invert (&X);
  gegl_matrix3_multiply (&Y, &X, &T->matrix);
}

void
npd_apply_transformation (NPDMatrix *T,
                          NPDPoint  *src,
                          NPDPoint  *dest)
{
  gdouble x = src->x, y = src->y;
  gegl_matrix3_transform_point (&T->matrix, &x, &y);
  dest->x = x; dest->y = y;
}
