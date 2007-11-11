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

#ifndef __GEGL_DEBUG_RECT_VISITOR_H__
#define __GEGL_DEBUG_RECT_VISITOR_H__

#include "graph/gegl-visitor.h"

G_BEGIN_DECLS


#define GEGL_TYPE_DEBUG_RECT_VISITOR            (gegl_debug_rect_visitor_get_type ())
#define GEGL_DEBUG_RECT_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DEBUG_RECT_VISITOR, GeglDebugRectVisitor))
#define GEGL_DEBUG_RECT_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DEBUG_RECT_VISITOR, GeglDebugRectVisitorClass))
#define GEGL_IS_DEBUG_RECT_VISITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DEBUG_RECT_VISITOR))
#define GEGL_IS_DEBUG_RECT_VISITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DEBUG_RECT_VISITOR))
#define GEGL_DEBUG_RECT_VISITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DEBUG_RECT_VISITOR, GeglDebugRectVisitorClass))


typedef struct _GeglDebugRectVisitorClass GeglDebugRectVisitorClass;

struct _GeglDebugRectVisitor
{
  GeglVisitor  parent_instance;
};

struct _GeglDebugRectVisitorClass
{
  GeglVisitorClass  parent_class;
};


GType   gegl_debug_rect_visitor_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GEGL_DEBUG_RECT_VISITOR_H__ */
