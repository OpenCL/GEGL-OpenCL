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

#ifndef __GEGL_INTROSPECTION_SUPPORT_H__
#define __GEGL_INTROSPECTION_SUPPORT_H__

#include <glib-object.h>

#include <gegl-types.h>

 /**
 * gegl_format:
 * @format_name: A Babl format name, e.g. "RGBA float"
 *
 * Returns a value sutable to pass to the GeglBuffer constructor
 * or any other property that expects a Babl format.
 *
 * Return value: (transfer full) (allow-none): the format pointer
 */

GValue *
gegl_format (const char *format_name);

 /**
 * gegl_format_get_name:
 * @format: A Babl pointer
 *
 * Return value: (transfer none) (allow-none): the format name
 */

const gchar *
gegl_format_get_name (GValue *format);

 /**
 * gegl_color_get_components:
 * @color: (transfer none): a #GeglColor
 * @format: A Babl pointer
 * @components_length: (out): The length of the returned buffer
 *
 * Get the component values of the color in @format.
 *
 * Return value: (transfer full) (array length=components_length): The color components
 */
gdouble *
gegl_color_get_components (GeglColor *color, GValue *format, gint *components_length);

 /**
 * gegl_color_set_components:
 * @color: (transfer none): a #GeglColor
 * @format: A Babl pointer
 * @components: (transfer none) (array length=components_length): The color components.
 * @components_length: The length of the components array
 *
 * Set the color using the component values as @format.
 */
void
gegl_color_set_components (GeglColor *color, GValue *format, gdouble *components, gint components_length);

 /**
 * gegl_node_introspectable_get_bounding_box: (rename-to gegl_node_get_bounding_box)
 * @node: a #GeglNode
 *
 * Returns the position and dimensions of a rectangle spanning the area
 * defined by a node.
 *
 * Return value: (transfer full): pointer a #GeglRectangle
 */
GeglRectangle *
gegl_node_introspectable_get_bounding_box (GeglNode *node);

/**
 * gegl_node_introspectable_get_property: (rename-to gegl_node_get_property)
 * @node: the node to get a property from
 * @property_name: the name of the property to get
 *
 * Return value: (transfer full): pointer to a GValue containing the value of the property
 */

GValue * gegl_node_introspectable_get_property (GeglNode    *node,
                                                const gchar *property_name);

/**
 * gegl_buffer_introspectable_new: (rename-to gegl_buffer_new)
 * @format_name: The Babl format name for this buffer, e.g. "RGBA float"
 * @x: x origin of the buffer's extent
 * @y: y origin of the buffer's extent
 * @width: width of the buffer's extent
 * @height: height of the buffer's extent
 *
 * Create a new GeglBuffer with the given format and dimensions.
 */
GeglBuffer *    gegl_buffer_introspectable_new (const char *format_name,
                                                gint        x,
                                                gint        y,
                                                gint        width,
                                                gint        height);

/**
 * gegl_buffer_introspectable_get: (rename-to gegl_buffer_get)
 * @buffer: the buffer to retrieve data from.
 * @rect: the coordinates we want to retrieve data from.
 * @scale: sampling scale, 1.0 = pixel for pixel 2.0 = magnify, 0.5 scale down.
 * @format_name: (allow-none): the format to store data in, if NULL the format of the buffer is used.
 * @repeat_mode: how requests outside the buffer extent are handled.
 * Valid values: GEGL_ABYSS_NONE (abyss pixels are zeroed), GEGL_ABYSS_WHITE
 * (abyss pixels are white), GEGL_ABYSS_BLACK (abyss pixels are black),
 * GEGL_ABYSS_CLAMP (coordinates are clamped to the abyss rectangle),
 * GEGL_ABYSS_LOOP (buffer contents are tiled if outside of the abyss rectangle).
 * @data_length: (out): The length of the returned buffer
 *
 * Fetch a rectangular linear buffer of pixel data from the GeglBuffer.
 *
 * Return value: (transfer full) (array length=data_length): A copy of the requested data
 */
guchar *       gegl_buffer_introspectable_get (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               gdouble              scale,
                                               const gchar         *format_name,
                                               GeglAbyssPolicy      repeat_mode,
                                               guint               *data_length);


/**
 * gegl_buffer_introspectable_set: (rename-to gegl_buffer_set)
 * @buffer: the buffer to modify.
 * @rect: the rectangle to write.
 * @format_name: the format of the input data.
 * @src: (transfer none) (array length=src_length): pixel data to write to @buffer.
 * @src_length: the lenght of src in bytes
 *
 * Store a linear raster buffer into the GeglBuffer.
 */
void           gegl_buffer_introspectable_set (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               const gchar         *format_name,
                                               const guchar        *src,
                                               gint                 src_length);
#endif /* __GEGL_INTROSPECTION_SUPPORT_H__ */

