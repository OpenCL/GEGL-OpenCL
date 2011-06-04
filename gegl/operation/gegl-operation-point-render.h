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

/* GeglOperationPointRender
 * The point-render base class is a specialized source operation, where the render is done
 * in small piece to lower the need to do copies. It's dedicated to operation which may be
 * rendered in pieces, like pattern generation.
 */

#ifndef __GEGL_OPERATION_POINT_RENDER_H__
#define __GEGL_OPERATION_POINT_RENDER_H__

#include "gegl-operation-source.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_POINT_RENDER            (gegl_operation_point_render_get_type ())
#define GEGL_OPERATION_POINT_RENDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_RENDER, GeglOperationPointRender))
#define GEGL_OPERATION_POINT_RENDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_RENDER, GeglOperationPointRenderClass))
#define GEGL_IS_OPERATION_POINT_RENDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_POINT_RENDER))
#define GEGL_IS_OPERATION_POINT_RENDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_POINT_RENDER))
#define GEGL_OPERATION_POINT_RENDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_RENDER, GeglOperationPointRenderClass))

typedef struct _GeglOperationPointRender  GeglOperationPointRender;
struct _GeglOperationPointRender
{
  GeglOperationSource parent_instance;
};

typedef struct _GeglOperationPointRenderClass GeglOperationPointRenderClass;
struct _GeglOperationPointRenderClass
{
  GeglOperationSourceClass parent_class;

  gboolean (* process) (GeglOperation       *self,      /* for parameters    */
                        void                *out_buf,   /* output buffer     */
                        glong                samples,   /* number of samples */
                        const GeglRectangle *roi,       /* can be used if position is of importance*/
                        gint                 level);
  gpointer              pad[4];
};

GType gegl_operation_point_render_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
