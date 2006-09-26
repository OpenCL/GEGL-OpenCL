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
 * Copyright 2003 Calvin Williamson
 */

#ifndef __GEGL_GRAPH_H__
#define __GEGL_GRAPH_H__

#include "gegl-object.h"

G_BEGIN_DECLS


#define GEGL_TYPE_GRAPH            (gegl_graph_get_type ())
#define GEGL_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_GRAPH, GeglGraph))
#define GEGL_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_GRAPH, GeglGraphClass))
#define GEGL_IS_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_GRAPH))
#define GEGL_IS_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_GRAPH))
#define GEGL_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_GRAPH, GeglGraphClass))


typedef struct _GeglGraphClass GeglGraphClass;

struct _GeglGraph
{
  GeglObject parent_instance;

  /*< private >*/
  GList    *children;
};

struct _GeglGraphClass
{
  GeglObjectClass  parent_class;
};


GType      gegl_graph_get_type         (void) G_GNUC_CONST;

GeglNode * gegl_graph_add_child        (GeglGraph    *self,
                                        GeglNode     *child);
GeglNode * gegl_graph_remove_child     (GeglGraph    *self,
                                        GeglNode     *child);
GeglNode * gegl_graph_get_nth_child    (GeglGraph    *self,
                                        gint          n);
GList    * gegl_graph_get_children     (GeglGraph    *self);
void       gegl_graph_remove_children  (GeglGraph    *self);
gint       gegl_graph_get_num_children (GeglGraph    *self);
GeglNode * gegl_graph_new_node         (GeglNode     *self,
                                        const gchar  *first_property_name,
                                        ...);
GeglNode * gegl_graph_create_node      (GeglNode     *self,
                                        const gchar  *operation);
GeglNode * gegl_graph_input            (GeglNode     *graph,
                                        const gchar  *name);
GeglNode * gegl_graph_output           (GeglNode     *graph,
                                        const gchar  *name);
G_END_DECLS

#endif /* __GEGL_GRAPH_H__ */
