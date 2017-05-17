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
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

#include <glib-object.h>

#include "gegl-enums.h"

G_BEGIN_DECLS

#define GEGL_AUTO_ROWSTRIDE 0

typedef enum
{
  GEGL_PARAM_PAD_OUTPUT = 1 << G_PARAM_USER_SHIFT,
  GEGL_PARAM_PAD_INPUT  = 1 << (G_PARAM_USER_SHIFT + 1)
} GeglPadType;

typedef enum
{
  GEGL_BLIT_DEFAULT  = 0,
  GEGL_BLIT_CACHE    = 1 << 0,
  GEGL_BLIT_DIRTY    = 1 << 1
} GeglBlitFlags;


typedef struct _GeglConfig GeglConfig;
GType gegl_config_get_type (void) G_GNUC_CONST;
#define GEGL_TYPE_CONFIG            (gegl_config_get_type ())
#define GEGL_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CONFIG, GeglConfig))
#define GEGL_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CONFIG))

typedef struct _GeglSampler       GeglSampler;
typedef struct _GeglCurve         GeglCurve;
typedef struct _GeglPath          GeglPath;
typedef struct _GeglColor         GeglColor;
typedef struct _GeglAudioFragment GeglAudioFragment;

typedef struct _GeglRectangle GeglRectangle;

struct _GeglRectangle
{
  gint x;
  gint y;
  gint width;
  gint height;
};
GType gegl_rectangle_get_type (void) G_GNUC_CONST;
#define GEGL_TYPE_RECTANGLE   (gegl_rectangle_get_type())

#define  GEGL_RECTANGLE(x,y,w,h) (&((GeglRectangle){(x), (y),   (w), (h)}))

typedef struct _GeglTileBackend GeglTileBackend;

typedef struct _GeglOperationContext GeglOperationContext;

typedef struct _GeglBuffer  GeglBuffer;
GType gegl_buffer_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_BUFFER    (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_IS_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglBuffer, g_object_unref)

typedef struct _GeglOperation  GeglOperation;
GType gegl_operation_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_OPERATION    (gegl_operation_get_type ())
#define GEGL_OPERATION(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION, GeglOperation))
#define GEGL_IS_OPERATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION))
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglOperation, g_object_unref)

typedef struct _GeglNode  GeglNode;
GType gegl_node_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_NODE    (gegl_node_get_type())
#define GEGL_NODE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
#define GEGL_IS_NODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE))
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglNode, g_object_unref)

typedef struct _GeglProcessor  GeglProcessor;
GType gegl_processor_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_PROCESSOR    (gegl_processor_get_type())
#define GEGL_PROCESSOR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PROCESSOR, GeglProcessor))
#define GEGL_IS_PROCESSOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PROCESSOR))
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglProcessor, g_object_unref)

typedef struct _GeglRandom  GeglRandom;
GType gegl_random_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_RANDOM    (gegl_random_get_type())

G_END_DECLS

#endif /* __GEGL_TYPES_H__ */
