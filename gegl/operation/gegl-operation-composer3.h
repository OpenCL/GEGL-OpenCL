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

#ifndef __GEGL_OPERATION_COMPOSER3_H__
#define __GEGL_OPERATION_COMPOSER3_H__

#include "gegl-operation.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_COMPOSER3            (gegl_operation_composer3_get_type ())
#define GEGL_OPERATION_COMPOSER3(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_COMPOSER3, GeglOperationComposer3))
#define GEGL_OPERATION_COMPOSER3_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_COMPOSER3, GeglOperationComposer3Class))
#define GEGL_IS_OPERATION_COMPOSER3(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_COMPOSER3))
#define GEGL_IS_OPERATION_COMPOSER3_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_COMPOSER3))
#define GEGL_OPERATION_COMPOSER3_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_COMPOSER3, GeglOperationComposer3Class))

typedef struct _GeglOperationComposer3  GeglOperationComposer3;
struct _GeglOperationComposer3
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationComposer3Class GeglOperationComposer3Class;
struct _GeglOperationComposer3Class
{
  GeglOperationClass parent_class;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *input,
                        GeglBuffer          *aux,
                        GeglBuffer          *aux2,
                        GeglBuffer          *output,
                        const GeglRectangle *result,
                        gint                 level);
  gpointer              pad[4];
};

GType gegl_operation_composer3_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
