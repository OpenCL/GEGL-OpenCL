/* This file is an image processing operation for GEGL
 *
 * sc-context-private.h
 * Copyright (C) 2012 Barak Itkin <lightningismyname@gmail.com>
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

#ifndef __GEGL_SC_CONTEXT_PRIVATE_H__
#define __GEGL_SC_CONTEXT_PRIVATE_H__

#include <gegl.h>
#include <poly2tri-c/refine/refine.h>

#include "sc-outline.h"
#include "sc-context.h"
#include "sc-sample.h"

typedef struct
{
  GHashTable         *pt2col;
  gboolean            is_valid;
} GeglScRenderCache;

struct _GeglScContext
{
  GeglScOutline      *outline;
  GeglRectangle       mesh_bounds;
  P2trMesh           *mesh;

  GeglScMeshSampling *sampling;

  gboolean            cache_uvt;
  GeglBuffer         *uvt;

  GeglScRenderCache  *render_cache;
};

#define GEGL_SC_BABL_UVT_TYPE   (babl_type_new ("uvt", "bits", sizeof (P2trUVT) * 8, NULL))
#define GEGL_SC_BABL_UVT_FORMAT (babl_format_n (GEGL_SC_BABL_UVT_TYPE, 1))

#endif
