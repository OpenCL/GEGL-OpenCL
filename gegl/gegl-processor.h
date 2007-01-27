/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås
 */

#ifndef __GEGL_PROCESSOR_H__
#define __GEGL_PROCESSOR_H__

#include "gegl-types.h"

GeglProcessor *gegl_node_new_processor      (GeglNode      *node,
                                             GeglRectangle *rectangle);
void           gegl_processor_set_rectangle (GeglProcessor *processor,
                                             GeglRectangle *rectangle);
gboolean       gegl_processor_work          (GeglProcessor *processor,
                                             gdouble       *progress);
void           gegl_processor_destroy       (GeglProcessor *processor);

#endif /* __GEGL_PROCESSOR_H__ */
