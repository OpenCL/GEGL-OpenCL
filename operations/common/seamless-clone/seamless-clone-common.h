/* This file is an image processing operation for GEGL
 *
 * seamless-clone-common.h
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

#ifndef __SEAMLESS_CLONE_COMMON_H__
#define __SEAMLESS_CLONE_COMMON_H__

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "poly2tri-c/render/mesh-render.h"

#include "find-outline.h"
#include "make-mesh.h"

typedef struct {
  GeglRectangle   bg_rect;
  GeglBuffer     *fg_buf;
  GeglBuffer     *bg_buf;
  ScMeshSampling *sampling;
  GHashTable     *pt2col;
  int             xoff;
  int             yoff;
} ScColorComputeInfo;

typedef struct {
  GeglRectangle      mesh_bounds;
  P2tRTriangulation *mesh;
  ScMeshSampling    *sampling;
  GeglBuffer        *uvt;
  ScOutline         *outline;
} ScCache;

gboolean   sc_render_seamless (GeglBuffer          *bg,
                               GeglBuffer          *fg,
                               int                  fg_xoff,
                               int                  fg_yoff,
                               GeglBuffer          *dest,
                               const GeglRectangle *dest_rect,
                               const ScCache       *cache);
                               
ScCache*   sc_generate_cache (GeglBuffer          *fg,
                              const GeglRectangle *extents,
                              int                  max_refine_steps);

void       sc_cache_free     (ScCache *cache);

#define SC_BABL_UVT_TYPE   (babl_type_new ("uvt", "bits", sizeof (P2tRuvt) * 8, NULL))
#define SC_BABL_UVT_FORMAT (babl_format_n (SC_BABL_UVT_TYPE, 3))

#endif
