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
 */

#ifndef __GEGL_VISITOR_H__
#define __GEGL_VISITOR_H__

G_BEGIN_DECLS


#define GEGL_TYPE_VISITOR               (gegl_visitor_get_type ())
#define GEGL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITOR, GeglVisitor))
#define GEGL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VISITOR, GeglVisitorClass))
#define GEGL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITOR))
#define GEGL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VISITOR))
#define GEGL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VISITOR, GeglVisitorClass))


typedef struct _GeglVisitorClass GeglVisitorClass;

struct _GeglVisitor
{
  GObject     parent_instance;
  gpointer    context_id;

  GSList     *visits_list;
  GHashTable *hash;
};

struct _GeglVisitorClass
{
  GObjectClass        parent_class;

  void (* visit_pad)  (GeglVisitor  *self,
                       GeglPad      *pad);
  void (* visit_node) (GeglVisitor  *self,
                       GeglNode     *node);
};


GType    gegl_visitor_get_type         (void) G_GNUC_CONST;

GSList * gegl_visitor_get_visits_list (GeglVisitor   *self);
void     gegl_visitor_reset           (GeglVisitor   *self);
void     gegl_visitor_visit_visitable (GeglVisitor   *self,
                                       GeglVisitable *visitable);
void     gegl_visitor_visit_pad       (GeglVisitor   *self,
                                       GeglPad       *pad);
void     gegl_visitor_visit_node      (GeglVisitor   *self,
                                       GeglNode      *node);
void     gegl_visitor_dfs_traverse    (GeglVisitor   *self,
                                       GeglVisitable *visitable);
void     gegl_visitor_bfs_traverse    (GeglVisitor   *self,
                                       GeglVisitable *visitable);


G_END_DECLS

#endif /* __GEGL_VISITOR_H__ */
