/* This file is an image processing operation for GEGL
 *
 * sc-outline.c
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
 * in this file allows to find the outline of a given image.
 *
 * Terminology:
 *
 *   Opaque  - A pixel with alpha of at least a given threshold
 *   Island  - An opaque pixel with 8 non-opaque neighbors
 *   Edge    - An opaque pixel with at least 1 (out of 8) non-opaque
 *             neighbors
 *   Area    - A continuos region (using 8 neighbor connectivity) of
 *             pixels with the same opacity state
 *   Outline - A non-repeating sequence of edge pixels where each pixel
 *             is a neighbor of the previous, and the first pixel is a
 *             neighbor of the last pixel
 *
 * Currently, the logic in this file implements an algorithm for
 * finding one outline. If more than one outline exists (may happen due
 * to non-opaque areas inside opaque areas ("holes") or due to the
 * existance of more than one opaque area in the image)
 * NOTE: Island pixels are ignored in the outline finding algorithm
 */

#include <gegl.h>
#include "sc-outline.h"
#include "sc-common.h"

static inline void
gegl_sc_point_copy_to (const GeglScPoint *src,
                       GeglScPoint       *dst)
{
  dst->x              = src->x;
  dst->y              = src->y;
  dst->outside_normal = src->outside_normal;
}

static inline GeglScPoint*
gegl_sc_point_copy (const GeglScPoint *src)
{
  GeglScPoint *self = g_slice_new (GeglScPoint);
  gegl_sc_point_copy_to (src, self);
  return self;
}

static inline GeglScPoint*
gegl_sc_point_move (const GeglScPoint *src,
                    GeglScDirection    t,
                    GeglScPoint       *dst)
{
  dst->x = src->x + GEGL_SC_DIRECTION_XOFFSET (t, 1);
  dst->y = src->y + GEGL_SC_DIRECTION_YOFFSET (t, 1);
  return dst;
}

static inline gboolean
gegl_sc_point_eq (const GeglScPoint *pt1,
                  const GeglScPoint *pt2)
{
  return pt1->x == pt2->x && pt1->y == pt2->y;
}

static inline gboolean
is_opaque (const GeglRectangle *search_area,
           GeglBuffer          *buffer,
           const Babl          *format,
           gdouble              threshold,
           const GeglScPoint   *pt)
{
  GeglScColor col[GEGL_SC_COLORA_CHANNEL_COUNT];

  if (! gegl_sc_point_in_rectangle (pt->x, pt->y, search_area))
    return FALSE;

  gegl_buffer_sample (buffer, pt->x, pt->y, NULL, col, format,
      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return col[GEGL_SC_COLOR_ALPHA_INDEX] >= threshold;
}

/* This function assumes the pixel is opaque! */
static inline gboolean
is_opaque_island (const GeglRectangle *search_area,
                  GeglBuffer          *buffer,
                  const Babl          *format,
                  gdouble              threshold,
                  const GeglScPoint   *pt)
{
  gint i;
  GeglScPoint temp;

  for (i = 0; i < 8; ++i)
    if (is_opaque (search_area, buffer, format, threshold, gegl_sc_point_move (pt, i, &temp)))
      return FALSE;

  return TRUE;
}

/* This function checks whether a pixel is opaque, is not an island and
 * also is an edge pixel */
static inline gboolean
is_valid_edge (const GeglRectangle *search_area,
               GeglBuffer          *buffer,
               const Babl          *format,
               gdouble              threshold,
               const GeglScPoint   *pt)
{
  gint i;
  GeglScPoint temp;

  if (! is_opaque (search_area, buffer, format, threshold, pt))
    return FALSE;

  for (i = 0; i < GEGL_SC_DIRECTION_COUNT; ++i)
    if (is_opaque (search_area, buffer, format, threshold, gegl_sc_point_move (pt, i, &temp)))
      return FALSE;

  return TRUE;
}

/**
 * This function takes an opaque edge pixel which MUST NOT BE AN ISLAND
 * and returns the next edge pixel, going clock-wise around the outline
 * of the area.
 *
 * Finding the next edge pixel is based on the following concept:
 * - If we are going clock-wise around the outline, then the circular
 *   sector whose center is the current pixel and it's two radii (in
 *   clock-wise order) are the line to the previous outline pixel and
 *   the line to the next outline pixel must contain only non-opaque
 *   pixels!
 * - So, use this property to go clockwise inside this circular sector
 *   untill you find an opaque pixel. That pixel must then be the next
 *   edge pixel (when going in clock-wise direction)!
 */
static inline GeglScDirection
walk_cw (const GeglRectangle *search_area,
         GeglBuffer          *buffer,
         const Babl          *format,
         gdouble              threshold,
         const GeglScPoint   *cur_pt,
         GeglScDirection      dir_from_prev,
         GeglScPoint         *next_pt)
{
  GeglScDirection dir_to_prev = GEGL_SC_DIRECTION_OPPOSITE (dir_from_prev);
  GeglScDirection dir_to_next = GEGL_SC_DIRECTION_CW (dir_to_prev);

  gegl_sc_point_move (cur_pt, dir_to_next, next_pt);

  while (! is_opaque (search_area, buffer, format, threshold, next_pt))
    {
      dir_to_next = GEGL_SC_DIRECTION_CW (dir_to_next);
      gegl_sc_point_move (cur_pt, dir_to_next, next_pt);
    }

  return dir_to_next;
}

/**
 * This function scans the image from the top left corner (X,Y = 0),
 * going right (increasing X) and then down (increasing Y) until it
 * encounters the first opaque edge pixel which is not an island. It
 * the finds and returns the outline in which this edge pixel takes
 * a part.
 */
GeglScOutline*
gegl_sc_outline_find (const GeglRectangle *search_area,
                      GeglBuffer          *buffer,
                      gdouble              threshold,
                      gboolean            *ignored_islands)
{
  const Babl *format = babl_format("RGBA float");
  GeglScOutline *result = g_ptr_array_new ();

  gboolean found = FALSE;
  GeglScPoint current, next, *first;
  GeglScDirection to_next;

  gint row_max = search_area->x + search_area->width;
  gint col_max = search_area->y + search_area->height;

  for (current.y = search_area->y; current.y < row_max; ++current.y)
    {
      for (current.x = search_area->x; current.x < col_max; ++current.x)
        if (is_opaque (search_area, buffer, format, threshold, &current))
          {
            if (is_opaque_island (search_area, buffer, format, threshold, &current))
              {
                if (ignored_islands)
                  *ignored_islands = TRUE;
              }
            else
              {
                found = TRUE;
                break;
               }
          }

      if (found)
        break;
    }

  if (found)
    {
      current.outside_normal = GEGL_SC_DIRECTION_N;
      g_ptr_array_add (result, first = gegl_sc_point_copy (&current));

      to_next = walk_cw (search_area, buffer, format, threshold,
          &current, GEGL_SC_DIRECTION_E, &next);

      while (! gegl_sc_point_eq (&next, first))
        {
          next.outside_normal = GEGL_SC_DIRECTION_CW (GEGL_SC_DIRECTION_CW (to_next));
          g_ptr_array_add (result, gegl_sc_point_copy (&next));
          gegl_sc_point_copy_to (&next, &current);
          to_next = walk_cw (search_area, buffer, format, threshold,
                             &current, to_next, &next);
        }
    }

  return result;
}

/**
 * A function to sort points first by increasing Y values and then by
 * increasing X values
 */
static gint
gegl_sc_point_cmp (const GeglScPoint **pt1,
                   const GeglScPoint **pt2)
{
  if ((*pt1)->y < (*pt2)->y)
    return -1;
  else if ((*pt1)->y > (*pt2)->y)
    return +1;
  else
    {
      if ((*pt1)->x < (*pt2)->x)
        return -1;
      else if ((*pt1)->x > (*pt2)->x)
        return +1;
      else
        return 0;
    }
}

/**
 * This function checks whether a given outline is the only outline
 * inside the image. The logic of the function works like this:
 *
 * - Iterate over each row of pixels in the search area
 * - Use the even-odd technique to decide whether you are outside or
 *   inside the area defined by the outline
 * - If we found a non-opaque pixel inside the area or an opaque pixel
 *   outside the area, then there is more than one outline
 *
 * To efficiently test whether the current pixel is a part of the
 * outline, we will sort the outline pixels by increasing Y values and
 * then by increasing X values.
 * Since our scan of the image is by rows (increasing X) and going from
 * top to bottom, this will exactly match the sorting of the array,
 * allowing to check whether a pixel is in the outline in constant
 * (amortized) time!
 */
gboolean
gegl_sc_outline_check_if_single (const GeglRectangle *search_area,
                                 GeglBuffer          *buffer,
                                 gdouble              threshold,
                                 GeglScOutline       *existing)
{
  const Babl *format = babl_format("RGBA float");
  GPtrArray *sorted_points = g_ptr_array_sized_new (existing->len);
  gboolean not_single = FALSE;

  GeglScPoint current, *sorted_p;
  guint s_index;

  gint row_max = search_area->x + search_area->width;
  gint col_max = search_area->y + search_area->height;

  for (s_index = 0; s_index < existing->len; ++s_index)
    g_ptr_array_add (sorted_points, g_ptr_array_index (existing, s_index));
  g_ptr_array_sort (sorted_points, (GCompareFunc) gegl_sc_point_cmp);

  s_index = 0;
  sorted_p = (GeglScPoint*) g_ptr_array_index (sorted_points, s_index);

  for (current.y = search_area->y; current.y < row_max; ++current.y)
    {
      gboolean inside = FALSE;

      for (current.x = search_area->x; current.x < col_max; ++current.x)
        {
          gboolean hit, opaque;

          opaque = is_opaque (search_area, buffer, format, threshold, &current);
          hit = gegl_sc_point_eq (&current, sorted_p);

          if (hit && ! inside)
            {
              inside = TRUE;
              sorted_p = (GeglScPoint*) g_ptr_array_index (sorted_points, ++s_index);
              /* Prevent "leaving" the area in the next if statement */
              hit = FALSE;
            }

          if (inside != opaque
              && ! (opaque && is_opaque_island (search_area, buffer, format, threshold, &current)))
            {
              not_single = FALSE;
              break;
            }

          if (hit && inside)
            {
              inside = FALSE;
              sorted_p = (GeglScPoint*) g_ptr_array_index (sorted_points, ++s_index);
            }
        }

      if (not_single)
        break;
    }

  g_ptr_array_free (sorted_points, TRUE);
  return ! not_single;
}

guint
gegl_sc_outline_length (GeglScOutline *self)
{
  return ((GPtrArray*) self)->len;
}

gboolean
gegl_sc_outline_equals (GeglScOutline *a,
                        GeglScOutline *b)
{
  if (a == b) /* Includes the case were both are NULL */
    return TRUE;
  else if ((a == NULL) != (b == NULL))
    return FALSE;
  else if (gegl_sc_outline_length (a) != gegl_sc_outline_length (b))
    return FALSE;
  else
    {
      guint n = gegl_sc_outline_length (a);
      guint i;
      for (i = 0; i < n; i++)
        {
          const GeglScPoint *pA = (GeglScPoint*) g_ptr_array_index (a, i);
          const GeglScPoint *pB = (GeglScPoint*) g_ptr_array_index (b, i);
          if (gegl_sc_point_cmp (&pA, &pB) != 0)
            return FALSE;
        }
      return TRUE;
    }
}

void
gegl_sc_outline_free (GeglScOutline *self)
{
  GPtrArray *real = (GPtrArray*) self;
  gint i;
  for (i = 0; i < real->len; i++)
    g_slice_free (GeglScPoint, g_ptr_array_index (self, i));
  g_ptr_array_free (real, TRUE);
}
