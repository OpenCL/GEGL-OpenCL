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

#ifndef __NPD_DEBUG_H__
#define __NPD_DEBUG_H__

#include "npd_common.h"
#include <glib.h>

void             npd_print_model                (NPDModel        *model,
                                                 gboolean         print_control_points);
void             npd_print_hidden_model         (NPDHiddenModel  *hm,
                                                 gboolean         print_bones,
                                                 gboolean         print_overlapping_points);
void             npd_print_bone                 (NPDBone         *bone);
void             npd_print_point                (NPDPoint        *point,
                                                 gboolean         print_details);
void             npd_print_overlapping_points   (NPDOverlappingPoints *op);

#endif	/* __NPD_DEBUG_H__ */
