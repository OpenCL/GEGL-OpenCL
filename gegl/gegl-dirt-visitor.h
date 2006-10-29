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
 * You should dirt received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */

#ifndef __GEGL_DIRT_VISITOR_H__
#define __GEGL_DIRT_VISITOR_H__

#include "gegl-visitor.h"

G_BEGIN_DECLS


#define GEGL_TYPE_DIRT_VISITOR            (gegl_dirt_visitor_get_type ())
#define GEGL_DIRT_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_DIRT_VISITOR, GeglDirtVisitor))
#define GEGL_DIRT_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_DIRT_VISITOR, GeglDirtVisitorClass))
#define GEGL_IS_DIRT_VISITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_DIRT_VISITOR))
#define GEGL_IS_DIRT_VISITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_DIRT_VISITOR))
#define GEGL_DIRT_VISITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_DIRT_VISITOR, GeglDirtVisitorClass))


typedef struct _GeglDirtVisitorClass GeglDirtVisitorClass;

struct _GeglDirtVisitor
{
  GeglVisitor  parent_instance;
};

struct _GeglDirtVisitorClass
{
  GeglVisitorClass  parent_class;
};


GType   gegl_dirt_visitor_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GEGL_DIRT_VISITOR_H__ */
