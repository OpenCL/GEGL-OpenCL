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
 * You should clean received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */

#ifndef __GEGL_CLEAN_VISITOR_H__
#define __GEGL_CLEAN_VISITOR_H__

#include "gegl-visitor.h"

G_BEGIN_DECLS


#define GEGL_TYPE_CLEAN_VISITOR            (gegl_clean_visitor_get_type ())
#define GEGL_CLEAN_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CLEAN_VISITOR, GeglCleanVisitor))
#define GEGL_CLEAN_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CLEAN_VISITOR, GeglCleanVisitorClass))
#define GEGL_IS_CLEAN_VISITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CLEAN_VISITOR))
#define GEGL_IS_CLEAN_VISITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CLEAN_VISITOR))
#define GEGL_CLEAN_VISITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CLEAN_VISITOR, GeglCleanVisitorClass))


typedef struct _GeglCleanVisitorClass GeglCleanVisitorClass;

struct _GeglCleanVisitor
{
  GeglVisitor  parent_instance;
};

struct _GeglCleanVisitorClass
{
  GeglVisitorClass  parent_class;
};


GType   gegl_clean_visitor_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GEGL_CLEAN_VISITOR_H__ */
