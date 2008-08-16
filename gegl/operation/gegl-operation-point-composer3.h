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
 * Copyright 2006 Øyvind Kolås
 */

#ifndef __GEGL_OPERATION_POINT_COMPOSER3_H__
#define __GEGL_OPERATION_POINT_COMPOSER3_H__

#include "gegl-operation-composer3.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_POINT_COMPOSER3            (gegl_operation_point_composer3_get_type ())
#define GEGL_OPERATION_POINT_COMPOSER3(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER3, GeglOperationPointComposer3))
#define GEGL_OPERATION_POINT_COMPOSER3_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER3, GeglOperationPointComposer3Class))
#define GEGL_IS_OPERATION_POINT_COMPOSER3(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER3))
#define GEGL_IS_OPERATION_POINT_COMPOSER3_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER3))
#define GEGL_OPERATION_POINT_COMPOSER3_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_COMPOSER3, GeglOperationPointComposer3Class))

typedef struct _GeglOperationPointComposer3  GeglOperationPointComposer3;
struct _GeglOperationPointComposer3
{
  GeglOperationComposer3 parent_instance;

  /*< private >*/
};

typedef struct _GeglOperationPointComposer3Class GeglOperationPointComposer3Class;
struct _GeglOperationPointComposer3Class
{
  GeglOperationComposer3Class parent_class;

  gboolean (* process) (GeglOperation       *self,      /* for parameters      */
                        void                *in,
                        void                *aux,
                        void                *aux2,
                        void                *out,
                        glong                samples, /* number of samples   */
                        const GeglRectangle *roi      /* rectangular region in output buffer */
                        ); 

};

GType gegl_operation_point_composer3_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
