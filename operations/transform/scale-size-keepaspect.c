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
 *           2017 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (x, _("X"), -1.0)
    description (_("Horizontal size"))
    ui_range (0.0, 20.0)
    value_range (-9000.0, 9000.0)

property_double (y, _("Y"), -1.0)
    description (_("Vertical size"))
    ui_range (0.0, 20.0)
    value_range (-9000.0, 9000.0)

#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    scale_size_keepaspect
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "scale-size-keepaspect.c"

#include "gegl-op.h"

#include <math.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  GeglOperation *operation  = GEGL_OPERATION (op);
  GeglRectangle  in_rect = {0,0,0,0};
  gdouble height_over_width = 1.0;

  if (gegl_operation_source_get_bounding_box (operation, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (operation, "input");

  // Avoid divide-by-zero
  if(in_rect.width < 1)
    in_rect.width = 1;
  if(in_rect.height < 1)
    in_rect.height = 1;

  height_over_width = in_rect.height/(gdouble)in_rect.width;
  if (o->x <= 0.0 && o->y <= 0.0) {
    // No dimensions specified, pass through
    matrix->coeff [0][0] = 1.0;
    matrix->coeff [1][1] = 1.0;
  } else if (o->x <= 0.0 && o->y > 0.0) {
    // X free, Y specified
    const gdouble target_x = o->y / height_over_width;
    matrix->coeff [0][0] = target_x / (gdouble) in_rect.width;
    matrix->coeff [1][1] = o->y / (gdouble) in_rect.height;
  } else if (o->y <= 0.0 && o->x > 0.0) {
    // Y free, X specified
    const gdouble target_y = o->x * height_over_width;
    matrix->coeff [0][0] = o->x / (gdouble) in_rect.width;
    matrix->coeff [1][1] = target_y / (gdouble) in_rect.height;
  } else {
    // Fully specified
    matrix->coeff [0][0] = o->x / (gdouble) in_rect.width;
    matrix->coeff [1][1] = o->y / (gdouble) in_rect.height;
  }
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
    "name", "gegl:scale-size-keepaspect",
    "title", _("Scale size keep aspect"),
    "categories", "transform",
    "reference-hash", "e9586912651e1837414339221413d2b3",
    "description", _("Scales the buffer to a size, preserving aspect ratio"),
    "reference-chain", "load path=images/standard-input.png scale-size-keepaspect x=140 y=-1",
    NULL);
}

#endif
