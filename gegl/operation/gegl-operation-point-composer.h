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

/* GeglOperationPointComposer
 * A baseclass for composer functions where the output pixels' values depends only on
 * the values of the single corresponding input and aux pixels.
 */

#ifndef __GEGL_OPERATION_POINT_COMPOSER_H__
#define __GEGL_OPERATION_POINT_COMPOSER_H__

#include "gegl-operation-composer.h"

#include "opencl/gegl-cl.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_POINT_COMPOSER            (gegl_operation_point_composer_get_type ())
#define GEGL_OPERATION_POINT_COMPOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposer))
#define GEGL_OPERATION_POINT_COMPOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))
#define GEGL_IS_OPERATION_POINT_COMPOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER))
#define GEGL_IS_OPERATION_POINT_COMPOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER))
#define GEGL_OPERATION_POINT_COMPOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))

typedef struct _GeglOperationPointComposer  GeglOperationPointComposer;
struct _GeglOperationPointComposer
{
  GeglOperationComposer parent_instance;

  /*< private >*/
};

typedef struct _GeglOperationPointComposerClass GeglOperationPointComposerClass;
struct _GeglOperationPointComposerClass
{
  GeglOperationComposerClass parent_class;

  gboolean (* process) (GeglOperation       *self,      /* for parameters      */
                        void                *in,
                        void                *aux,
                        void                *out,
                        glong                samples, /* number of samples   */
                        const GeglRectangle *roi,     /* rectangular region in output buffer */
                        gint                 level);

  cl_int   (* cl_process) (GeglOperation       *self,
                           cl_mem               in_tex,
                           cl_mem               aux_tex,
                           cl_mem               out_tex,
                           size_t               global_worksize,
                           const GeglRectangle *roi,
                           gint                 level);
  gpointer                 pad[4];
};

GType gegl_operation_point_composer_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
