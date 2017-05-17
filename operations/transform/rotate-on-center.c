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
 * Copyright 2014 Jon Nordby, The Grid <jononor@gmail.com>
 *           2015 Vilson Vieira, The Grid <vilson@void.cc>
 *           2017 Øyvind Kolås
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (degrees, _("Degrees"), 0.0)
    description(_("Angle to rotate (counter-clockwise)"))
    ui_range (-180.0, 180.0)

property_double (origin_x, _("origin-x") , 0.0)
    description(_("Ignored. Always uses center of input buffer"))
property_double (origin_y, _("origin-y") , 0.0)
    description(_("Ignored. Always uses center of input buffer"))

#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    rotate_on_center
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "rotate-on-center.c"

#include "gegl-op.h"

#include <math.h>

static void
generate_matrix (GeglMatrix3 *matrix,
                 gdouble degrees,
                 gdouble x,
                 gdouble y,
                 gdouble width,
                 gdouble height)
{
  gint i;
  gdouble radians = degrees * (2 * G_PI / 360.0),
          tx = 0.0,
          ty = 0.0,
          tcoords[4][2];
  /*
   * Find coordinates of each corner of the bounding box around buffer,
   * after the requested rotation.
   */
  tcoords[0][0] = - x*cos (radians) - y*sin (radians);
  tcoords[0][1] =   x*sin (radians) - y*cos (radians);

  tcoords[1][0] =   width*cos (radians) + tcoords[0][0];
  tcoords[1][1] = - width*sin (radians) + tcoords[0][1];

  tcoords[2][0] =   width*cos (radians) + height*sin (radians) + tcoords[0][0];
  tcoords[2][1] = - width*sin (radians) + height*cos (radians) + tcoords[0][1];

  tcoords[3][0] =   height*sin (radians) + tcoords[0][0];
  tcoords[3][1] =   height*cos (radians) + tcoords[0][1];

  /*
   * Find translation needed to make the bounding box stays in the positive
   * quadrant.
   */
  for (i=0; i<G_N_ELEMENTS (tcoords); i++) {
    tx = MIN (tx, tcoords[i][0]);
    ty = MIN (ty, tcoords[i][1]);
  }

  /*
   * Define an affine matrix that:
   * 1. Translates buffer's center to origin
   * 2. Rotates buffer by given degrees
   * 3. Translates to the positive quadrant
   */
  matrix->coeff [0][0] =  cos (radians);
  matrix->coeff [0][1] =  sin (radians);
  matrix->coeff [0][2] = -tx - x*cos (radians) - y*sin (radians);

  matrix->coeff [1][0] = -sin (radians);
  matrix->coeff [1][1] =  cos (radians);
  matrix->coeff [1][2] = -ty + x*sin (radians) - y*cos (radians);

  matrix->coeff [2][0] = 0;
  matrix->coeff [2][1] = 0;
  matrix->coeff [2][2] = 1;
}


static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  GeglOperation  *operation  = GEGL_OPERATION (op);
  GeglRectangle   in_rect = {0.0,0.0,0.0,0.0};

  if (gegl_operation_source_get_bounding_box (operation, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (operation, "input");

  // Avoid divide-by-zero
  if (in_rect.width < 1)
    in_rect.width = 1;
  if (in_rect.height < 1)
    in_rect.height = 1;

  generate_matrix (matrix,
                   o->degrees,
                   in_rect.width,
                   in_rect.height,
                   in_rect.width,
                   in_rect.height);
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
    "name",       "gegl:rotate-on-center",
    "title",      _("Rotate on center"),
    "categories", "transform",
    "reference-hash", "d1f7774133373578693604d7eb561a82",
    "reference-chain", "load path=images/standard-input.png rotate-on-center degrees=30.0 clip-to-input=true",
    "description", _("Rotate the buffer around its center, taking care of possible offsets."),
    NULL);
}

#endif
