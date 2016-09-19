/* This file is an image processing operation for GEGL
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
 * Copyright 1997 Brian Degenhardt <bdegenha@ucsd.edu>
 * Copyright 1997 Federico Mena Quinter <quartic@polloux.fciencias.unam.mx>
 * Copyright 2012 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int  (shift, _("Shift"), 5)
    description(_("Maximum amount to shift"))
    value_range (0, 200)
    ui_meta    ("unit", "pixel-distance")

property_enum (direction, _("Shift direction"),
    GeglOrientation, gegl_orientation,
    GEGL_ORIENTATION_HORIZONTAL)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     shift
#define GEGL_OP_C_SOURCE shift.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  const Babl              *format;

  if (o->direction == GEGL_ORIENTATION_HORIZONTAL)
    {
      op_area->left   = o->shift;
      op_area->right  = o->shift;
      op_area->top    = 0;
      op_area->bottom = 0;
    }
  else if (o->direction == GEGL_ORIENTATION_VERTICAL)
    {
      op_area->top    = o->shift;
      op_area->bottom = o->shift;
      op_area->left   = 0;
      op_area->right  = 0;
    }

  format = gegl_operation_get_source_format (operation, "input");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties    *o = GEGL_PROPERTIES (operation);
  gint           size, i, pos;
  GeglRectangle  dst_rect;


  if (o->direction == GEGL_ORIENTATION_HORIZONTAL)
    {
      size = result->height;
      dst_rect.width  = result->width;
      dst_rect.height = 1;
      pos = result->y;
    }
  else
    {
      size = result->width;
      dst_rect.width  = 1;
      dst_rect.height = result->height;
      pos = result->x;
    }

  dst_rect.x = result->x;
  dst_rect.y = result->y;

  for (i = 0; i < size; i++)
    {
      GeglRectangle src_rect;
      gint shift = gegl_random_int_range (o->rand, i + pos, 0, 0, 0,
                                          -o->shift, o->shift + 1);

      if (o->direction == GEGL_ORIENTATION_HORIZONTAL)
        {
          dst_rect.y = i + result->y;
          src_rect = dst_rect;
          src_rect.x = result->x + shift;
        }
      else
        {
          dst_rect.x = i + result->x;
          src_rect = dst_rect;
          src_rect.y = result->y + shift;
        }

      gegl_buffer_copy (input, &src_rect, GEGL_ABYSS_CLAMP,
                        output, &dst_rect);
    }

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shift",
    "title",       _("Shift"),
    "categories",  "distort",
    "license",     "GPL3+",
    "description", _("Shift each row or column of pixels by a random amount"),
    NULL);
}

#endif
