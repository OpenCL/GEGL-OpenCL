/* This file is an image processing operation for GEGL
 *
 * find-outline.h
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

#ifndef __SEAMLESS_CLONE_FIND_OUTLINE_H__
#define __SEAMLESS_CLONE_FIND_OUTLINE_H__

#include <gegl.h>

typedef struct  {
  gint x, y;
} ScPoint;

/* Define a type for the outline to distinguish it from all the other
 * pointer arrays in the code of the seamless cloning. Also allow later
 * to pass it transparently to other places and free it, without
 * depending on the actual representation of this type. 
 */
typedef GPtrArray ScOutline;

ScOutline* outline_find_ccw (GeglRectangle *rect, gfloat *pixels);

void       sc_outline_free  (ScOutline *self);

#endif
