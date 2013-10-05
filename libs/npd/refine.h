/*
 * This file is part of N-point image deformation library.
 *
 * N-point image deformation library is free software: you can
 * redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * N-point image deformation library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with N-point image deformation library.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#ifndef __REFINE_H__
#define	__REFINE_H__

#include "npd_common.h"

void         npd_swap_ints (gint *i,
                            gint *j);
gboolean     npd_is_edge_empty (NPDImage *image,
                                NPDPoint *p1,
                                NPDPoint *p2);
GHashTable  *npd_find_edges (NPDModel *model);
void         npd_sort_overlapping_points (NPDOverlappingPoints *op);
void         npd_handle_corner_and_segment (NPDOverlappingPoints *center,
                                            NPDOverlappingPoints *p1,
                                            NPDOverlappingPoints *p2,
                                            GHashTable *changed_ops);
void         npd_handle_3_neighbors (NPDOverlappingPoints *center,
                                     NPDOverlappingPoints *p1,
                                     NPDOverlappingPoints *p2,
                                     NPDOverlappingPoints *p3,
                                     GHashTable *changed_ops);
void         npd_cut_edges (NPDHiddenModel *hm, GHashTable *edges);

#endif	/* __REFINE_H__ */
