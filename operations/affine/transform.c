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

gegl_chant_string (transform, "", _("Transformation string"))

#else

#define GEGL_CHANT_NAME transform
#define GEGL_CHANT_DESCRIPTION  _("Transforms the group (used by svg).")
#define GEGL_CHANT_SELF "transform.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (GeglChantOperation *op,
               Matrix3        matrix)
{

  gchar *p = strchr (op->transform, '(');
  gfloat a;
  gfloat b;
  if (!p) return;
  p++;
  a = strtod(p, &p);
  if (!p) return;
  p = strchr (op->transform, ',');
  if (!p) return;
  p++;
  b = strtod (p, &p);
  if (!p) return;

  matrix [0][2] = a;
  matrix [1][2] = b;
}

#endif
