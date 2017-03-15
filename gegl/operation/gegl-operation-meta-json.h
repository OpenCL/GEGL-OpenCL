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
 * Copyright 2014 Jon Nordby <jononor@gmail.com>
 */

/* GeglOperationMetaJsn
 * Used for GEGL operations that are implemented as a sub-graph in .json
 */

#ifndef __GEGL_OPERATION_META_JSON_H__
#define __GEGL_OPERATION_META_JSON_H__

#include "gegl-operation-meta.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_META_JSON            (gegl_operation_meta_get_type ())
#define GEGL_OPERATION_META_JSON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_META_JSON, GeglOperationMetaJson))
#define GEGL_OPERATION_META_JSON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_META_JSON, GeglOperationMetaJsonClass))
#define GEGL_IS_OPERATION_META_JSON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_META_JSON))
#define GEGL_IS_OPERATION_META_JSON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_META_JSON))
#define GEGL_OPERATION_META_JSON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_META_JSON, GeglOperationMetaJsonClass))

typedef struct _GeglOperationMetaJson  GeglOperationMetaJson;
struct _GeglOperationMetaJson
{
  GeglOperationMeta parent_instance;

  gpointer pad[4];
};

typedef struct _GeglOperationMetaJsonClass GeglOperationMetaJsonClass;
struct _GeglOperationMetaJsonClass
{
  GeglOperationMetaClass parent_class;
  gpointer           pad[4];
};

GType gegl_operation_meta_json_get_type         (void) G_GNUC_CONST;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglOperationMetaJson, g_object_unref)

G_END_DECLS

#endif
