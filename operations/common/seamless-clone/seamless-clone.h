/* This file is an image processing operation for GEGL
 *
 * seamless-clone.h
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

#ifndef __SEAMLESS_CLONE_H__
#define __SEAMLESS_CLONE_H__

#define TO_CAIRO(col) ((col)/2+0.5)
//#define FROM_CAIRO(col) (((col)-0.5)*2)
//#define TO_CAIRO(col) (col)
#define FROM_CAIRO(col) ((col - 128)*2)

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"

#include "find-outline.h"
#include "make-mesh.h"

#endif
