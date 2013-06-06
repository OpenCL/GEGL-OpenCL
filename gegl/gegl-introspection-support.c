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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "graph/gegl-node.h"
#include "gegl-introspection-support.h"

GValue *
gegl_node_introspectable_get_property (GeglNode    *node,
                                       const gchar *property_name)
{
  GValue *result = g_new0 (GValue, 1);

  gegl_node_get_property (node, property_name, result);

  return result;
}

GeglBuffer *
gegl_buffer_introspectable_new (const char *format_name,
                                gint        x,
                                gint        y,
                                gint        width,
                                gint        height)
{
  const Babl *format = NULL;

  if (format_name)
    format = babl_format (format_name);

  if (!format)
    format = babl_format ("RGBA float");

  return g_object_new (GEGL_TYPE_BUFFER,
                       "x", x,
                       "y", y,
                       "width", width,
                       "height", height,
                       "format", format,
                       NULL);
}
