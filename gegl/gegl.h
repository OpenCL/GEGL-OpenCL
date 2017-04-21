/* This file is the public GEGL API
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
 * 2000-2008 © Calvin Williamson, Øyvind Kolås.
 */

#ifndef __GEGL_H__
#define __GEGL_H__

#include <glib-object.h>
#include <babl/babl.h>


#define __GEGL_H_INSIDE__

#include <gegl-types.h>

#include <gegl-buffer.h>
#include <gegl-color.h>
#include <gegl-curve.h>
#include <gegl-path.h>
#include <gegl-matrix.h>
#include <gegl-utils.h>
#include <gegl-operations-util.h>
#include <gegl-init.h>
#include <gegl-version.h>
#include <gegl-random.h>
#include <gegl-node.h>
#include <gegl-processor.h>
#include <gegl-apply.h>

#undef __GEGL_H_INSIDE__

/***
 * The GEGL API:
 *
 * This document is both a tutorial and a reference for the C API of GEGL.
 * The concepts covered in this reference should also be applicable when
 * using other languages.
 *
 * The core API of GEGL isn't frozen yet and feedback regarding its use as
 * well as the clarity of this documentation is most welcome.
 */

G_BEGIN_DECLS

/***
 * Introduction:
 *
 * Algorithms created with GEGL are expressed as graphs of nodes. The nodes
 * have associated image processing operations. A node has output and input
 * pads which can be connected. By connecting these nodes in chains a set of
 * image operation filters and combinators can be applied to the image data.
 *
 * To make GEGL process data you request a rectangular region of a node's
 * output pad to be rendered into a provided linear buffer of any (supported
 * by babl) pixel format. GEGL uses information provided by the nodes to
 * determine the smallest buffers needed at each stage of processing.
 */

#define GEGL_ALIGNED __restrict__ __attribute__((__aligned__ (16)))

G_END_DECLS
#endif  /* __GEGL_H__ */
