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

gboolean     npd_is_edge_empty    (NPDImage *image,
                                   gint      X1,
                                   gint      Y1,
                                   gint      X2,
                                   gint      Y2);
GList**      npd_find_edges       (NPDImage *image,
                                   gint      count_x,
                                   gint      count_y,
                                   gint      square_size);
GList*       npd_cut_edges        (GList   **edges,
                                   gint      ow,
                                   gint      oh);

#endif	/* __REFINE_H__ */
