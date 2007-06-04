/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 */

#ifndef __GEGL_PLUGIN_H__
#define __GEGL_PLUGIN_H__

#include <glib-object.h>

#include <gegl/gegl-types.h>
#include <gegl/gegl-color.h>
#include <gegl/gegl-connection.h>
#include <gegl/gegl-curve.h>
#include <gegl/gegl-graph.h>
#include <gegl/gegl-init.h>
#include <gegl/gegl-node.h>
#include <gegl/gegl-pad.h>
#include <gegl/gegl-utils.h>
#include <gegl/gegl-visitable.h>
#include <gegl/gegl-visitor.h>
#include <gegl/buffer/gegl-buffer.h>
#include <gegl/gegl-curve.h>
#include <gegl/gegl-xml.h>
#include <gegl/operation/gegl-operation.h>

#define GEGL_INTERNAL /* to avoid conflicts, when we include gegl.h just
                         to get warnings about conflicts in the prototypes
                         of functions. */
#include <gegl.h>
#include <gegl-paramspecs.h>
#endif  /* __GEGL_H__ */
