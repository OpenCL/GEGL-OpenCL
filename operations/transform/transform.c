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

gegl_chant_string (transform, "", _("Transformation string"))

#else

#define GEGL_CHANT_NAME transform
#define GEGL_CHANT_DESCRIPTION  _("Transforms the group (used by svg).")
#define GEGL_CHANT_SELF "transform.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglChantOperation *chant = GEGL_CHANT_OPERATION (op);

  gegl_matrix3_parse_string (matrix, chant->transform);
}

#endif
