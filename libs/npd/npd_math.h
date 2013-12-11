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

#ifndef __NPD_MATH_H__
#define	__NPD_MATH_H__

#include "npd_common.h"
#include <gegl-matrix.h>

typedef GeglMatrix3 NPDMatrix;

#define NPD_EPSILON 0.00001

void        npd_compute_affinity     (NPDPoint  *p11,
                                      NPDPoint  *p21,
                                      NPDPoint  *p31,
                                      NPDPoint  *p12,
                                      NPDPoint  *p22,
                                      NPDPoint  *p32,
                                      NPDMatrix *T);
void        npd_apply_transformation (NPDMatrix *T,
                                      NPDPoint  *src,
                                      NPDPoint  *dest);
gboolean    npd_equal_floats_epsilon (gfloat     a,
                                      gfloat     b,
                                      gfloat     epsilon);
gboolean    npd_equal_floats         (gfloat     a,
                                      gfloat     b);
gfloat      npd_SED                  (NPDPoint  *p1,
                                      NPDPoint  *p2);

#endif	/* __NPD_MATH_H__ */
