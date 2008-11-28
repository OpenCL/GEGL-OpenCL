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
#include <glib/gi18n-lib.h>


#if GEGL_CHANT_PROPERTIES

gegl_chant_double (degrees, -G_MAXDOUBLE, G_MAXDOUBLE, 0.,
                   _("Angle to rotate (clockwize)"))

#else

#define GEGL_CHANT_NAME rotate
#define GEGL_CHANT_DESCRIPTION  _("Rotate the buffer around the specified origin.")
#define GEGL_CHANT_SELF "rotate.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (GeglChantOperation *op,
               GeglMatrix3        matrix)
{
  gdouble radians = op->degrees * (2 * G_PI / 360.);

  matrix [0][0] = matrix [1][1] = cos (radians);
  matrix [0][1] = sin (radians);
  matrix [1][0] = - matrix [0][1];
}

#endif
