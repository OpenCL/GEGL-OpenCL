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
 * Copyright 2006 Øyvind Kolås
 */
#ifndef _GEGL_OPERATION_COMPOSER_H__
#define _GEGL_OPERATION_COMPOSER_H__

#include <glib-object.h>
#include "gegl-types.h"
#include "buffer/gegl-buffer.h"
#include "gegl-operation.h"
#include "gegl-pad.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_OPERATION_COMPOSER               (gegl_operation_composer_get_type ())
#define GEGL_OPERATION_COMPOSER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposer))
#define GEGL_OPERATION_COMPOSER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposerClass))
#define GEGL_OPERATION_COMPOSER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposerClass))

typedef struct _GeglOperationComposer  GeglOperationComposer;
struct _GeglOperationComposer
{
    GeglOperation       operation;

    /*< private >*/
    GeglBuffer *input;
    GeglBuffer *aux;
    GeglBuffer *output;
};

typedef struct _GeglOperationComposerClass GeglOperationComposerClass;
struct _GeglOperationComposerClass
{
   GeglOperationClass operation_class;
   gboolean (*process) (GeglOperation *self,
                        const gchar   *output_pad);
};

GType gegl_operation_composer_get_type (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
