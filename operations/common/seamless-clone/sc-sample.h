/* This file is an image processing operation for GEGL
 *
 * sc-sample.h
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

#ifndef __GEGL_SC_SAMPLE_H__
#define __GEGL_SC_SAMPLE_H__

#include <glib.h>
#include <poly2tri-c/refine/refine.h>

#include "sc-outline.h"

typedef struct {
  /** Should the point behind this list be sampled directly?        */
  gboolean   direct_sample;
  /** An array of GeglScPoint* (pointers) of the points to sample       */
  GPtrArray *points;
  /** An array of weights to assign to the samples from the points  */
  GArray    *weights;
  /** The total weight of the samples, used to normalize the result */
  gdouble    total_weight;
} GeglScSampleList;

typedef GHashTable GeglScMeshSampling;

/**
 * Compute the list of points that should be sampled in order to
 * compute the color assigned to the given point in the color
 * difference mesh.
 */
GeglScSampleList*   gegl_sc_sample_list_compute   (GeglScOutline     *outline,
                                                   gdouble            x,
                                                   gdouble            y);

GeglScSampleList*   gegl_sc_sample_list_direct    (void);
/**
 * Free an GeglScSampleList object created by gegl_sc_sample_list_compute
 */
void                gegl_sc_sample_list_free      (GeglScSampleList   *self);

/**
 * Compute the sample lists for all the points in a given mesh
 */
GeglScMeshSampling* gegl_sc_mesh_sampling_compute (GeglScOutline      *outline,
                                                   P2trMesh           *mesh);

/**
 * Free an GeglScMeshSampling object created by gegl_sc_mesh_sampling_compute
 */
void                gegl_sc_mesh_sampling_free    (GeglScMeshSampling *self);

#endif
