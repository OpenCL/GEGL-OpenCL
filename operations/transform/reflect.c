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
 *           2017 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (x, _("X"), 0.0)
    description (_("Direction vector's X component"))
    ui_range (-100.0, 100.0)

property_double (y, _("Y"), 0.0)
    description (_("Direction vector's Y component"))
    ui_range (-100.0, 100.0)


#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    reflect
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "reflect.c"

#include "gegl-op.h"

#include <math.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  const gdouble ux = o->x;
  const gdouble uy = o->y;

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

#include <stdio.h>

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  OpTransformClass   *transform_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  transform_class = OP_TRANSFORM_CLASS (klass);
  transform_class->create_matrix = create_matrix;

  gegl_operation_class_set_keys (operation_class,
    "name", "gegl:reflect",
    "title", _("Reflect"),
    "categories", "transform",
    "description", _("Reflect an image about a line, whose direction is "
                     "specified by the vector that is defined by the "
                     "x and y properties. "),
    "reference-chain", "load path=images/standard-input.png reflet origin-x=100 origin-y=100 x=0.3 y=1.2",
    NULL);
}

#endif
