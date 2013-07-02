/*
 * This file is part of n-point image deformation library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#include "npd_math.h"
#include <math.h>

gboolean
npd_equal_floats (gfloat a,
                  gfloat b)
{
  return npd_equal_floats_epsilon (a, b, NPD_EPSILON);
}

gboolean
npd_equal_floats_epsilon (gfloat a,
                          gfloat b,
                          gfloat epsilon)
{
  return fabs (a - b) < epsilon;
}

gfloat
npd_SED (NPDPoint *p1,
         NPDPoint *p2)
{
  gint dx = p1->x - p2->x;
  gint dy = p1->y - p2->y;
  return dx * dx + dy * dy;
}