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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_shift_direction)
  enum_value (GEGL_SHIFT_DIRECTION_HORIZONTAL, "Horizontal")
  enum_value (GEGL_SHIFT_DIRECTION_VERTICAL,   "Vertical")
gegl_chant_register_enum_end (GeglShiftDirection)

gegl_chant_int  (shift, _("Shift"),
                 1, 200, 5,
                 _("Maximum amount to shift"))

gegl_chant_seed (seed, _("Seed"),
                 _("Random seed"))

gegl_chant_enum (direction, _("Direction"),
                 GeglShiftDirection, gegl_shift_direction,
                 GEGL_SHIFT_DIRECTION_HORIZONTAL,
                 _("Shift direction"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "shift.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  if (o->chant_data)
    {
      g_array_free (o->chant_data, TRUE);
      o->chant_data = NULL;
    }

  if (o->direction == GEGL_SHIFT_DIRECTION_HORIZONTAL)
    {
      op_area->left   = o->shift;
      op_area->right  = o->shift;
      op_area->top    = 0;
      op_area->bottom = 0;
    }
  else if (o->direction == GEGL_SHIFT_DIRECTION_VERTICAL)
    {
      op_area->top    = o->shift;
      op_area->bottom = o->shift;
      op_area->left   = 0;
      op_area->right  = 0;
    }

  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GMutex mutex;

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  gfloat *src_buf;
  gfloat *dst_buf;
  GeglRectangle src_rect;

  gint x = 0; /* initial x                   */
  gint y = 0; /*           and y coordinates */

  gfloat *in_pixel;
  gfloat *out_pixel;

  gint n_pixels = result->width * result->height;
  gint i;
  gint shift;
  gint s = o->shift;

  GArray *offsets;

  GeglRectangle *boundary;

  gint array_size = 0;
  gint r;

  /* calculate offsets once */
  g_mutex_lock (&mutex);
  if (!o->chant_data)
    {
      boundary = gegl_operation_source_get_bounding_box (operation, "input");

      if (boundary)
        {
          offsets = g_array_new (FALSE, FALSE, sizeof (gint));

          if (o->direction == GEGL_SHIFT_DIRECTION_HORIZONTAL)
            {
              array_size = boundary->height;
            }
          else if (o->direction == GEGL_SHIFT_DIRECTION_VERTICAL)
            {
              array_size = boundary->width;
            }

          for (i = 0; i < array_size; i++)
            {
              r = gegl_random_int_range (o->seed, i, 0, 0, 0, -s, s + 1);
              g_array_append_val (offsets, r);
            }
          o->chant_data = offsets;
        }
    }
  g_mutex_unlock (&mutex);

  offsets = o->chant_data;

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_slice_alloc (src_rect.width * src_rect.height * 4 * sizeof (gfloat));
  dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof (gfloat));

  in_pixel = src_buf;
  out_pixel = dst_buf;

  gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  while (n_pixels--)
    {
      /* select the desired input pixel */
      if (o->direction == GEGL_SHIFT_DIRECTION_HORIZONTAL)
        {
          shift = g_array_index (offsets, gint, result->y + y);
          in_pixel = src_buf + 4*(src_rect.width * y + s + x + shift);
        }
      else if (o->direction == GEGL_SHIFT_DIRECTION_VERTICAL)
        {
          shift = g_array_index (offsets, gint, result->x + x);
          in_pixel = src_buf + 4*(src_rect.width * (y + s + shift) + x);
        }

      /* copy pixel */
      for (i = 0; i < 4; i++)
        {
          *out_pixel = *in_pixel;
          in_pixel ++;
          out_pixel ++;
        }
      x++;
      if (x == result->width)
        {
          x = 0;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_slice_free1 (src_rect.width * src_rect.height * 4 * sizeof (gfloat), src_buf);
  g_slice_free1 (result->width * result->height * 4 * sizeof (gfloat), dst_buf);

  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_array_free (o->chant_data, TRUE);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shift",
    "categories",  "distort",
    "description", _("Shift each row or column of pixels by a random amount"),
    NULL);
}

#endif
