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
 * Copyright 2012 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE, 100.0,
                   _("Horizontal size"))
gegl_chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE, 100.0,
                   _("Vertical size"))

#else

#define GEGL_CHANT_NAME scale_size
#define GEGL_CHANT_OPERATION_NAME "gegl:scale-size"
#define GEGL_CHANT_DESCRIPTION _("Scales the buffer according to a size.")
#define GEGL_CHANT_SELF "scale-size.c"
#include "chant.h"

#include <math.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglChantOperation *chant = GEGL_CHANT_OPERATION (op);
  GeglOperation *operation  = GEGL_OPERATION (op);
  GeglRectangle  in_rect = {0,0,0,0};

  if (gegl_operation_source_get_bounding_box (operation, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (operation, "input");

  if(in_rect.width < 1)
    in_rect.width = 1;
  if(in_rect.height < 1)
    in_rect.height = 1;

  matrix->coeff [0][0] = chant->x / (gdouble) in_rect.width;
  matrix->coeff [1][1] = chant->y / (gdouble) in_rect.height;
}

#endif
