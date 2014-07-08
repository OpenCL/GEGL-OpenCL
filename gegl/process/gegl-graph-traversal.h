/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Daniel Sabo
 */
#ifndef __GEGL_GRAPH_TRAVERSAL_H__
#define __GEGL_GRAPH_TRAVERSAL_H__

typedef struct _GeglGraphTraversal GeglGraphTraversal;

GeglGraphTraversal *gegl_graph_build            (GeglNode            *node);
void                gegl_graph_rebuild          (GeglGraphTraversal  *path,
                                                 GeglNode            *node);
void                gegl_graph_free             (GeglGraphTraversal  *path);

void                gegl_graph_prepare          (GeglGraphTraversal  *path);
void                gegl_graph_prepare_request  (GeglGraphTraversal  *path,
                                                 const GeglRectangle *roi,
                                                 gint                 level);
GeglBuffer         *gegl_graph_process          (GeglGraphTraversal  *path,
                                                 gint                 level);

GeglRectangle       gegl_graph_get_bounding_box (GeglGraphTraversal  *path);

#endif /* __GEGL_GRAPH_TRAVERSAL_H__ */
