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

#ifndef __GEGL_PROCESSOR_H__
#define __GEGL_PROCESSOR_H__

#include "gegl-types-internal.h"

G_BEGIN_DECLS

#define GEGL_PROCESSOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PROCESSOR, GeglProcessorClass))
#define GEGL_IS_PROCESSOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PROCESSOR))
#define GEGL_PROCESSOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PROCESSOR, GeglProcessorClass))
/* The rest is in gegl-types.h */


typedef struct _GeglProcessorClass GeglProcessorClass;

struct _GeglProcessorClass
{
  GObjectClass  parent_class;
};


GType          gegl_processor_get_type      (void) G_GNUC_CONST;

GeglProcessor *gegl_node_new_processor      (GeglNode            *node,
                                             const GeglRectangle *rectangle);
void           gegl_processor_set_rectangle (GeglProcessor       *processor,
                                             const GeglRectangle *rectangle);
gboolean       gegl_processor_work          (GeglProcessor       *processor,
                                             gdouble             *progress);
G_END_DECLS

#endif /* __GEGL_PROCESSOR_H__ */
