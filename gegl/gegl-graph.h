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

/* graph related methods of gegl_node class */

G_BEGIN_DECLS

GeglNode * gegl_node_add_child        (GeglNode    *self,
                                       GeglNode    *child);
GeglNode * gegl_node_remove_child     (GeglNode    *self,
                                       GeglNode     *child);
GeglNode * gegl_node_get_nth_child    (GeglNode    *self,
                                       gint          n);
GList    * gegl_node_get_children     (GeglNode    *self);
void       gegl_node_remove_children  (GeglNode    *self);
gint       gegl_node_get_num_children (GeglNode    *self);

GeglNode * gegl_node_new              (void);

GeglNode * gegl_node_new_child         (GeglNode     *self,
                                       const gchar  *first_property_name,
                                       ...) G_GNUC_NULL_TERMINATED;
GeglNode * gegl_node_create_child      (GeglNode     *self,
                                       const gchar  *operation);
GeglNode * gegl_node_get_input_proxy           (GeglNode     *graph,
                                       const gchar  *name);
GeglNode * gegl_node_get_output_proxy          (GeglNode     *graph,
                                       const gchar  *name);
G_END_DECLS

#endif /* __GEGL_GRAPH_H__ */
