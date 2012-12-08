/* This file is an image processing operation for GEGL
 *
 * sc-outline.h
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

#ifndef __GEGL_SC_OUTLINE_H__
#define __GEGL_SC_OUTLINE_H__

#include <gegl.h>

/**
 * An enumeration for representing progress directions in 2 dimensions
 * <pre>
 *       0
 *
 *   7   N   1
 *       ^
 *       |
 * 6 W<--+-->E  2
 *       |           =====>X
 *       v           ||
 *   5   S   3       ||
 *                   vv
 *       4           Y
 * </pre>
 */
typedef enum {
  GEGL_SC_DIRECTION_N     = 0,
  GEGL_SC_DIRECTION_NE    = 1,
  GEGL_SC_DIRECTION_E     = 2,
  GEGL_SC_DIRECTION_SE    = 3,
  GEGL_SC_DIRECTION_S     = 4,
  GEGL_SC_DIRECTION_SW    = 5,
  GEGL_SC_DIRECTION_W     = 6,
  GEGL_SC_DIRECTION_NW    = 7,
  GEGL_SC_DIRECTION_COUNT = 8
} GeglScDirection;

#define GEGL_SC_DIRECTION_CW(d)       (((d) + 1) % 8)
#define GEGL_SC_DIRECTION_CCW(d)      (((d) + 7) % 8)
#define GEGL_SC_DIRECTION_OPPOSITE(d) (((d) + 4) % 8)

#define GEGL_SC_DIRECTION_IS_NORTH(d) (       \
  ((d) == GEGL_SC_DIRECTION_N)  ||            \
  ((d) == GEGL_SC_DIRECTION_NE) ||            \
  ((d) == GEGL_SC_DIRECTION_NW)               \
)

#define GEGL_SC_DIRECTION_IS_SOUTH(d) (       \
  ((d) == GEGL_SC_DIRECTION_S)  ||            \
  ((d) == GEGL_SC_DIRECTION_SE) ||            \
  ((d) == GEGL_SC_DIRECTION_SW)               \
)

#define GEGL_SC_DIRECTION_IS_EAST(d) (        \
  ((d) == GEGL_SC_DIRECTION_E)  ||            \
  ((d) == GEGL_SC_DIRECTION_NE) ||            \
  ((d) == GEGL_SC_DIRECTION_SE)               \
)

#define GEGL_SC_DIRECTION_IS_WEST(d) (        \
  ((d) == GEGL_SC_DIRECTION_W)  ||            \
  ((d) == GEGL_SC_DIRECTION_NW) ||            \
  ((d) == GEGL_SC_DIRECTION_SW)               \
)

#define GEGL_SC_DIRECTION_XOFFSET(d,s) (      \
  (GEGL_SC_DIRECTION_IS_EAST(d)) ? (s) :      \
    ((GEGL_SC_DIRECTION_IS_WEST(d)) ? -(s) :  \
      0)                                      \
)

#define GEGL_SC_DIRECTION_YOFFSET(d,s) (      \
  (GEGL_SC_DIRECTION_IS_SOUTH(d)) ? (s) :     \
    ((GEGL_SC_DIRECTION_IS_NORTH(d)) ? -(s) : \
      0)                                      \
)

/**
 * A struct for representing a point which belongs to an outline,
 * together with the direction which is the normal pointing outside
 * of the interior area defined by the outline
 */
typedef struct  {
  gint x, y;
  GeglScDirection outside_normal;
} GeglScPoint;

/* Define a type for the outline to distinguish it from all the other
 * pointer arrays in the code of the seamless cloning. Also allow later
 * to pass it transparently to other places and free it, without
 * depending on the actual representation of this type.
 */
typedef GPtrArray GeglScOutline;

GeglScOutline* gegl_sc_outline_find            (const GeglRectangle *rect,
                                                GeglBuffer          *pixels,
                                                gdouble              threshold,
                                                gboolean            *ignored_islands);

gboolean       gegl_sc_outline_check_if_single (const GeglRectangle *search_area,
                                                GeglBuffer          *buffer,
                                                gdouble              threshold,
                                                GeglScOutline       *existing);

guint          gegl_sc_outline_length          (GeglScOutline       *self);

gboolean       gegl_sc_outline_equals          (GeglScOutline       *a,
                                                GeglScOutline       *b);

void           gegl_sc_outline_free            (GeglScOutline       *self);

#endif
