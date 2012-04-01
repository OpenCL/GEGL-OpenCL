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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                   _("Horizontal scale factor"))
gegl_chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                   _("Vertical scale factor"))

#else

#define GEGL_CHANT_NAME scale
#define GEGL_CHANT_DESCRIPTION _("Scales the buffer.")
#define GEGL_CHANT_SELF "scale.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglChantOperation *chant = GEGL_CHANT_OPERATION (op);

  matrix->coeff [0][0] = chant->x;
  matrix->coeff [1][1] = chant->y;
}

#endif
