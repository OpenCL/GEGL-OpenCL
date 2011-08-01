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

void sc_free_sample_list (ScSampleList *self);

void
ComputeSampling (P2tRTriangulation  *T,
                 P2tRHashSet       **edgePts,
                 GHashTable        **sampling);

void
ComputeInnerSample (P2tRPoint  *X,
                    gpointer   *sampleData, /* the value of the key X in sampling */
                    GeglBuffer *input,
                    GeglBuffer *aux,
                    gfloat      dest[3]);

#endif
