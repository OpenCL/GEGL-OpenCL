/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */

#ifndef __GEGL_VISITOR_H__
#define __GEGL_VISITOR_H__

#include "gegl-object.h"
#include "gegl-property.h"
#include "gegl-node.h"

struct _GeglVisitable;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GEGL_TYPE_VISITOR               (gegl_visitor_get_type ())
#define GEGL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITOR, GeglVisitor))
#define GEGL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VISITOR, GeglVisitorClass))
#define GEGL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITOR))
#define GEGL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VISITOR))
#define GEGL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VISITOR, GeglVisitorClass))

typedef struct _GeglVisitor GeglVisitor;
struct _GeglVisitor 
{
   GeglObject object;

   GList * visits_list;
   GHashTable *hash;
};

typedef struct _GeglVisitorClass GeglVisitorClass;
struct _GeglVisitorClass 
{
   GeglObjectClass object_class;

   void (* visit_property)         (GeglVisitor *self,
                                    GeglProperty * property);
   void (* visit_node)             (GeglVisitor *self,
                                    GeglNode * node);
};

GType           gegl_visitor_get_type           (void); 
GList *         gegl_visitor_get_visits_list    (GeglVisitor *self);
void            gegl_visitor_visit_visitable    (GeglVisitor *self,
                                                     struct _GeglVisitable *visitable);
void            gegl_visitor_visit_property     (GeglVisitor * self,
                                                     GeglProperty *property);
void            gegl_visitor_visit_node         (GeglVisitor * self,
                                                     GeglNode *node);
void            gegl_visitor_dfs_traverse       (GeglVisitor * self, 
                                                     struct _GeglVisitable * visitable); 
void            gegl_visitor_bfs_traverse       (GeglVisitor * self, 
                                                     struct _GeglVisitable * visitable); 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
