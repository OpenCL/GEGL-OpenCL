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

#ifndef __NPD_DEFORMATION_H__
#define	__NPD_DEFORMATION_H__

#include "npd_common.h"

void  npd_compute_centroid_from_weighted_points
                                        (gint              num_of_points,
                                         NPDPoint          points[],
                                         gfloat            weights[],
                                         NPDPoint         *centroid);
void  npd_compute_centroid_of_overlapping_points
                                        (gint              num_of_points,
                                         NPDPoint         *points[],
                                         gfloat            weights[],
                                         NPDPoint         *centroid);
void  npd_compute_ARSAP_transformation  (gint              num_of_points,
                                         NPDPoint          reference_shape[],
                                         NPDPoint          current_shape[],
                                         gfloat            weights[],
                                         gboolean          ASAP);
void  npd_compute_ARSAP_transformations (NPDHiddenModel   *model);
void  npd_deform_model                  (NPDModel         *model,
                                         gint              rigidity);
void  npd_deform_model_once             (NPDModel         *model);
void  npd_deform_hidden_model_once      (NPDHiddenModel   *hidden_model);

#endif	/* __NPD_DEFORMATION_H__ */
