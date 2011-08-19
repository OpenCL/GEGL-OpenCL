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

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"

#include "find-outline.h"
#include "make-mesh.h"

typedef struct {
  ScOutline         *outline;
  P2tRTriangulation *mesh;
  GeglRectangle      mesh_bounds;
  ScMeshSampling    *sampling;
  GeglBuffer        *uvt;
} ScPreprocessResult;

#define sc_preprocess_new() (g_new0 (ScPreprocessResult, 1))

//inline void
//sc_preprocess_result_free (ScPreprocessResult *self)
//{
//  sc_mesh_sampling_free (self->sampling);
//  p2tr_triangulation_free (self->mesh);
//  sc_outline_free (self->outline);
//
//  g_free (self);
//}

#define babl_uvt_type   (babl_type_new ("uvt", "bits", sizeof (P2tRuvt) * 8, NULL))
#define babl_uvt_format (babl_format_n (babl_uvt_type, 3))

#endif
