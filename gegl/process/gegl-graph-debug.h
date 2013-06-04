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

#ifndef __GEGL_GRAPH_DEBUG_H__
#define __GEGL_GRAPH_DEBUG_H__

/**
 * gegl_graph_dump_outputs:
 * @node: The final node of the graph
 *
 * Dump the bounds and format of each node in the graph to stdout.
 */
void gegl_graph_dump_outputs (GeglNode *node);

/**
 * gegl_graph_dump_request:
 * @node: The final node of the graph
 * @roi: The request rectangle
 *
 * Dump the region that will be rendered for each node to fulfill
 * the request.
 */
void gegl_graph_dump_request (GeglNode *node, const GeglRectangle *roi);

#endif /* __GEGL_GRAPH_DEBUG_H__ */
