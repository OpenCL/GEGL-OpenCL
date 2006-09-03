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
#ifndef _GEGL_OPERATION_POINT_COMPOSER_H
#define _GEGL_OPERATION_POINT_COMPOSER_H

#include <glib-object.h>
#include "gegl-types.h"
#include "buffer/gegl-buffer.h"
#include "gegl-operation.h"
#include "gegl-operation-composer.h"
#include "gegl-pad.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_OPERATION_POINT_COMPOSER           (gegl_operation_point_composer_get_type ())
#define GEGL_OPERATION_POINT_COMPOSER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposer))
#define GEGL_OPERATION_POINT_COMPOSER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))
#define GEGL_OPERATION_POINT_COMPOSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))

typedef struct _GeglOperationPointComposer  GeglOperationPointComposer;
struct _GeglOperationPointComposer
{
    GeglOperationComposer  operation;
    Babl                  *format; /* the format that this instance is
                                      going to do it's processing in (
                                      the pixelformat aquired from the in-pad
                                      and used for computation) */
    Babl                  *aux_format; /* the format that the aux buffer
                                          will be in */
    gint                 samples;
    /*< private >*/
};

typedef struct _GeglOperationPointComposerClass GeglOperationPointComposerClass;
struct _GeglOperationPointComposerClass
{
   GeglOperationComposerClass operation_class;
   gboolean (*process) (GeglOperation *self,      /* for parameters      */
                        void          *in,
                        void          *aux,
                        void          *out,
                        glong          samples);  /* number of samples   */

};

GType gegl_operation_point_composer_get_type (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
