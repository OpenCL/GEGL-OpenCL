/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#ifdef CHANT_SELF

chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE, 1., "")
chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE, 1., "")

#else

#define CHANT_NAME shear
#define CHANT_SELF "shear.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (ChantInstance *op,
               Matrix3        matrix)
{
  matrix [0][1] = op->x;
  matrix [1][0] = op->y;
}

#endif
