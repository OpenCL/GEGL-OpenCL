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

#ifndef __GEGL_OPERATION_META_H__
#define __GEGL_OPERATION_META_H__

#include "gegl-operation.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_META            (gegl_operation_meta_get_type ())
#define GEGL_OPERATION_META(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_META, GeglOperationMeta))
#define GEGL_OPERATION_META_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_META, GeglOperationMetaClass))
#define GEGL_IS_OPERATION_META(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_META))
#define GEGL_IS_OPERATION_META_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_META))
#define GEGL_OPERATION_META_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_META, GeglOperationMetaClass))

typedef struct _GeglOperationMeta  GeglOperationMeta;
struct _GeglOperationMeta
{
  GeglOperation parent_instance;

  GSList       *redirects;
};

typedef struct _GeglOperationMetaClass GeglOperationMetaClass;
struct _GeglOperationMetaClass
{
  GeglOperationClass parent_class;
};


GType gegl_operation_meta_get_type         (void) G_GNUC_CONST;

void  gegl_operation_meta_redirect         (GeglOperation     *operation,
                                            const gchar       *name,
                                            GeglNode          *internal,
                                            const gchar       *internal_name);

void  gegl_operation_meta_property_changed (GeglOperationMeta *self,
                                            GParamSpec        *arg1,
                                            gpointer           user_data);

G_END_DECLS

#endif
