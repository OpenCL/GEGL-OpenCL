/* This file is an image processing operation for GEGL
 *
 * make-mesh.h
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

#ifndef __SEAMLESS_CLONE_MAKE_MESH_H__
#define __SEAMLESS_CLONE_MAKE_MESH_H__

typedef struct {
  GPtrArray *points;  /* An array of ScPoint* objects */
  GArray    *weights; /* An array of gdouble values */
  gdouble    total_weight;
} ScSampleList;

typedef GHashTable ScMeshSampling;

ScSampleList* sc_sample_list_compute (ScOutline *outline, gdouble Px, gdouble Py);
void          sc_sample_list_free    (ScSampleList *self);

ScMeshSampling* sc_mesh_sampling_compute (ScOutline *outline, P2tRTriangulation *mesh);
void            sc_mesh_sampling_free    (ScMeshSampling *self);

void
ComputeInnerSample (P2tRPoint  *X,
                    gpointer   *sampleData, /* the value of the key X in sampling */
                    GeglBuffer *input,
                    GeglBuffer *aux,
                    gfloat      dest[3]);

#endif
