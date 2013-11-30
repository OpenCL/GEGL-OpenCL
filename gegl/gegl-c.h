/* This file is the public GEGL API
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
 * 2000-2008 © Calvin Williamson, Øyvind Kolås.
 */

#ifndef __GEGL_C_H__
#define __GEGL_C_H__

G_BEGIN_DECLS

/**
 * gegl_node: (skip)
 * @op_type:  the type of operation to create
 * @first_property_name:
 * @...:
 *
 * Construct a GEGL node, connecting it to needed input nodes. The
 * returned node does not have a parent but a single reference it
 * is meant to be passed to gegl_graph () for gegl_graph () to assume
 * its ownership. This is syntactic sugar for use from C, similar
 * conveniences can easily be built externally in other languages.
 *
 * gegl_node(op_type, [key, value, [...]], NULL, [input, [aux]])
 *
 * Return value: (transfer full): a new Gegl node.
 */
GeglNode *gegl_node (const gchar *op_type,
                     const gchar *first_property_name,
                     ...) G_GNUC_DEPRECATED;


/**
 * gegl_graph: (skip)
 * @node: (transfer full): the end result of a composition created with gegl_node()
 *
 * Creates a GeglNode containing a free floating graph constructed
 * using gegl_node(). The GeglGraph adopts all the passed in nodes
 * making it sufficient to unref the resulting graph.
 *
 * gegl_graph (gegl_node ("gegl:over", NULL,
 *                        gegl_node (..), gegl_node (..)));
 *
 * Return value: (transfer full):a GeglNode graph.
 */
GeglNode *gegl_graph (GeglNode *node) G_GNUC_DEPRECATED;

G_END_DECLS

#endif  /* __GEGL_C_H__ */
