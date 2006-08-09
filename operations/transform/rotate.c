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

chant_double (degrees, -G_MAXDOUBLE, G_MAXDOUBLE, 0., "")

#else

#define CHANT_NAME rotate
#define CHANT_SELF "rotate.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (ChantInstance *op,
               Matrix3        matrix)
{
  gdouble radians = op->degrees * (2 * G_PI / 360.);

  matrix [0][0] = matrix [1][1] = cos (radians);
  matrix [0][1] = sin (radians);
  matrix [1][0] = - matrix [0][1];
}

#endif
