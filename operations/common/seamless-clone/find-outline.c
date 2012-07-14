/* This file is an image processing operation for GEGL
 *
 * find-outline.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file is part 1 of 3 in the seamless clone operation. The logic
 * in this file allows to find the outline of a given image. A pixel is
 * considered an outline pixel, if it's alpha is larger than 0.5 and one
 * of it's neighbors has alpha lower than that.
 *
 * Currently, the logic in this file allows for finding one outline with
 * no holes, and it currently assumse that only one "island" of high
 * alpha exists
 */

#include <gegl.h>


#include "seamless-clone.h"
#include <poly2tri-c/p2t/poly2tri.h>
#include <poly2tri-c/refine/refine.h>

static inline void
sc_point_copy_to (const ScPoint *src,
                  ScPoint       *dst)
{
  dst->x              = src->x;
  dst->y              = src->y;
  dst->outside_normal = src->outside_normal;
}

static inline ScPoint*
sc_point_copy (const ScPoint *src)
{
  ScPoint *self = g_slice_new (ScPoint);
  sc_point_copy_to (src, self);
  return self;
}

static inline ScPoint*
sc_point_move (const ScPoint *src,
               ScDirection    t,
               ScPoint       *dst)
{
  dst->x = src->x + SC_DIRECTION_XOFFSET (t, 1);
  dst->y = src->y + SC_DIRECTION_YOFFSET (t, 1);
  return dst;
}

static inline gboolean
sc_point_eq (const ScPoint *pt1,
             const ScPoint *pt2)
{
  return pt1->x == pt2->x && pt1->y == pt2->y;
}

static gint
sc_point_cmp (const ScPoint *pt1,
              const ScPoint *pt2)
{
  if (pt1->y < pt2->y)
    return -1;
  else if (pt1->y > pt2->y)
    return +1;
  else
    {
      if (pt1->x < pt2->x)
        return -1;
      else if (pt1->x > pt2->x)
        return +1;
      else
        return 0;
    }
}

static inline gboolean
in_range (gint val,
          gint min,
          gint max)
{
  return (min <= val) && (val <= max);
}

static inline gboolean
in_rectangle (const GeglRectangle *rect,
              const ScPoint       *pt)
{
  return in_range (pt->x, rect->x, rect->x + rect->width - 1)
    && in_range (pt->y, rect->y, rect->y + rect->height - 1);
}

static inline gboolean
is_opaque (const GeglRectangle *search_area,
           GeglBuffer          *buffer,
           const Babl          *format,
           const ScPoint       *pt)
{
  gfloat col[4] = {0, 0, 0, 0};

  if (! in_rectangle (search_area, pt))
    return FALSE;

  gegl_buffer_sample (buffer, pt->x, pt->y, NULL, col, format,
      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return col[3] >= 0.5f;
}

/* This function assumes the pixel is white! */
static inline gboolean
is_opaque_island (const GeglRectangle *search_area,
                  GeglBuffer          *buffer,
                  const Babl          *format,
                  const ScPoint       *pt)
{
  gint i;
  ScPoint temp;

  for (i = 0; i < 8; ++i)
    if (is_opaque (search_area, buffer, format, sc_point_move (pt, i, &temp)))
      return FALSE;

  return TRUE;
}

static inline ScDirection
walk_cw (const GeglRectangle *search_area,
         GeglBuffer          *buffer,
         const Babl          *format,
         const ScPoint       *cur_pt,
         ScDirection          dir_from_prev,
         ScPoint             *next_pt)
{
  ScDirection dir_to_prev = SC_DIRECTION_OPPOSITE (dir_from_prev);
  ScDirection dir_to_next = SC_DIRECTION_CW (dir_to_prev);

  sc_point_move (cur_pt, dir_to_next, next_pt);

  while (! is_opaque (search_area, buffer, format, next_pt))
    {
      dir_to_next = SC_DIRECTION_CW (dir_to_next);
      sc_point_move (cur_pt, dir_to_next, next_pt);
    }

  return dir_to_next;
}

ScOutline*
sc_outline_find (const GeglRectangle *search_area,
                 GeglBuffer          *buffer)
{
  const Babl *format = babl_format("RGBA float");
  ScOutline *result = g_ptr_array_new ();

  gboolean found = FALSE;
  ScPoint current, next, *first;
  ScDirection to_next;

  gint row_max = search_area->x + search_area->width;
  gint col_max = search_area->y + search_area->height;

  for (current.y = search_area->y; current.y < row_max; ++current.y)
    {
      for (current.x = search_area->x; current.x < col_max; ++current.x)
        if (is_opaque (search_area, buffer, format, &current)
            && ! is_opaque_island (search_area, buffer, format, &current))
          {
            found = TRUE;
            break;
          }

      if (found)
        break;
    }

  if (found)
    {
      current.outside_normal = SC_DIRECTION_N;
      g_ptr_array_add (result, first = sc_point_copy (&current));

      to_next = walk_cw (search_area, buffer, format, &current,
          SC_DIRECTION_E, &next);

      while (! sc_point_eq (&next, first))
        {
          next.outside_normal = SC_DIRECTION_CW(SC_DIRECTION_CW(to_next));
          g_ptr_array_add (result, sc_point_copy (&next));
          sc_point_copy_to (&next, &current);
          to_next = walk_cw (search_area, buffer, format, &current,
              to_next, &next);
        }
    }

  return result;
}

void
sc_outline_free (ScOutline *self)
{
  GPtrArray *real = (GPtrArray*) self;
  gint i;
  for (i = 0; i < real->len; i++)
    g_slice_free (ScPoint, g_ptr_array_index (self, i));
  g_ptr_array_free (real, TRUE);
}
