/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2007 Øyvind Kolås
 */

/* GeglOperationAreaFilter
 * The AreaFilter base class allows defining operations where the output data depends on a neighbourhood
 * with an input window that extends beyond the output window, the information about needed extra pixels
 * in different directions should be set up in the prepare callback for the operation.
*/

#ifndef __GEGL_OPERATION_AREA_FILTER_H__
#define __GEGL_OPERATION_AREA_FILTER_H__

#include "gegl-operation-filter.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_AREA_FILTER            (gegl_operation_area_filter_get_type ())
#define GEGL_OPERATION_AREA_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilter))
#define GEGL_OPERATION_AREA_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilterClass))
#define GEGL_IS_OPERATION_AREA_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_AREA_FILTER))
#define GEGL_IS_OPERATION_AREA_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_AREA_FILTER))
#define GEGL_OPERATION_AREA_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilterClass))

typedef struct _GeglOperationAreaFilter  GeglOperationAreaFilter;
struct _GeglOperationAreaFilter
{
  GeglOperationFilter parent_instance;

  gint                left;   /* extra pixels needed in each direction */
  gint                right;
  gint                top;
  gint                bottom;
};

typedef struct _GeglOperationAreaFilterClass GeglOperationAreaFilterClass;
struct _GeglOperationAreaFilterClass
{
  GeglOperationFilterClass parent_class;
  gpointer                 pad[4];
};

GType gegl_operation_area_filter_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
