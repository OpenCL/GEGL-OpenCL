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
  SC_DIRECTION_N  = 0,
  SC_DIRECTION_NE = 1,
  SC_DIRECTION_E  = 2,
  SC_DIRECTION_SE = 3,
  SC_DIRECTION_S  = 4,
  SC_DIRECTION_SW = 5,
  SC_DIRECTION_W  = 6,
  SC_DIRECTION_NW = 7
} ScDirection;

#define SC_DIRECTION_CW(d)       (((d)+1)%8)
#define SC_DIRECTION_CCW(d)      (((d)+7)%8)
#define SC_DIRECTION_OPPOSITE(d) (((d)+4)%8)

#define SC_DIRECTION_IS_NORTH(d) (       \
  ((d) == SC_DIRECTION_N)  ||            \
  ((d) == SC_DIRECTION_NE) ||            \
  ((d) == SC_DIRECTION_NW)               \
)

#define SC_DIRECTION_IS_SOUTH(d) (       \
  ((d) == SC_DIRECTION_S)  ||            \
  ((d) == SC_DIRECTION_SE) ||            \
  ((d) == SC_DIRECTION_SW)               \
)

#define SC_DIRECTION_IS_EAST(d) (        \
  ((d) == SC_DIRECTION_E)  ||            \
  ((d) == SC_DIRECTION_NE) ||            \
  ((d) == SC_DIRECTION_SE)               \
)

#define SC_DIRECTION_IS_WEST(d) (        \
  ((d) == SC_DIRECTION_W)  ||            \
  ((d) == SC_DIRECTION_NW) ||            \
  ((d) == SC_DIRECTION_SW)               \
)

#define SC_DIRECTION_XOFFSET(d,s) (      \
  (SC_DIRECTION_IS_EAST(d)) ? (s) :      \
    ((SC_DIRECTION_IS_WEST(d)) ? -(s) :  \
      0)                                 \
)

#define SC_DIRECTION_YOFFSET(d,s) (      \
  (SC_DIRECTION_IS_SOUTH(d)) ? (s) :     \
    ((SC_DIRECTION_IS_NORTH(d)) ? -(s) : \
      0)                                 \
)

/**
 * A struct for representing a point which belongs to an outline,
 * together with the direction which is the normal pointing outside
 * of the interior area defined by the outline
 */
typedef struct  {
  gint x, y;
  ScDirection outside_normal;
} ScPoint;

/* Define a type for the outline to distinguish it from all the other
 * pointer arrays in the code of the seamless cloning. Also allow later
 * to pass it transparently to other places and free it, without
 * depending on the actual representation of this type. 
 */
typedef GPtrArray ScOutline;

ScOutline* sc_outline_find            (const GeglRectangle *rect,
                                       GeglBuffer          *pixels,
                                       gboolean            *ignored_islands);

gboolean   sc_outline_check_if_single (const GeglRectangle *search_area,
                                       GeglBuffer          *buffer,
                                       ScOutline           *existing);

guint      sc_outline_length          (ScOutline *self);

void       sc_outline_free            (ScOutline *self);

#endif
