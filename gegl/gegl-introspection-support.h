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
 * Copyright 2013 Daniel Sabo
 */

 /*
  * This file contains alternate versions of functions to make them
  * more introspection friendly. They are not a public part of the
  * C API and should not be used outside of this file.
  */

#include <glib-object.h>

#include <gegl-types.h>

/**
 * gegl_node_introspectable_get_property:
 * @node: the node to get a property from
 * @property_name: the name of the property to get
 *
 * Rename to: gegl_node_get_property
 *
 * Return value: (transfer full): pointer to a GValue containing the value of the property
 */

GValue * gegl_node_introspectable_get_property (GeglNode    *node,
                                                const gchar *property_name);
