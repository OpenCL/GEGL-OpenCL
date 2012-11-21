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
 * Copyright 2006 Dominik Ernst
 *           2012 Nicolas Robidoux
 *
 * Reflect an image about a line, whose direction is specified by the
 * vector that is defined by the x and y properties.
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Direction vector's X component"))
gegl_chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Direction vector's Y component"))

#else

#define GEGL_CHANT_NAME reflect
#define GEGL_CHANT_SELF "reflect.c"
#include "chant.h"

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglChantOperation *chant = GEGL_CHANT_OPERATION (op);
  const gdouble ux = chant->x;
  const gdouble uy = chant->y;

  /*
   * There probably should be an assertion or check+fix that
   * length_squared != 0.
   */
  const gdouble length_squared = ux*ux + uy*uy;
  const gdouble two_over_length_squared = (gdouble) 2 / length_squared;

  matrix->coeff [0][0] = ux*ux * two_over_length_squared - (gdouble) 1;
  matrix->coeff [1][1] = uy*uy * two_over_length_squared - (gdouble) 1;
  matrix->coeff [0][1] = matrix->coeff [1][0] = ux*uy * two_over_length_squared;
}

#endif
