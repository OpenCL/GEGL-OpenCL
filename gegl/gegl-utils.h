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


#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

G_BEGIN_DECLS




/***
 * GeglRectangle:
 *
 * GeglRectangles are used in #gegl_node_get_bounding_box and #gegl_node_blit
 * for specifying rectangles.
 *
 * </p><pre>struct GeglRectangle
 * {
 *   gint x;
 *   gint y;
 *   gint width;
 *   gint height;
 * };</pre><p>
 *
 */

/**
 * gegl_rectangle_set:
 * @rectangle: a #GeglRectangle
 * @x: upper left x coordinate
 * @y: upper left y coordinate
 * @width: width in pixels.
 * @height: height in pixels.
 *
 * Sets the @x, @y, @width and @height on @rectangle.
 */
void        gegl_rectangle_set           (GeglRectangle       *rectangle,
                                          gint                 x,
                                          gint                 y,
                                          guint                width,
                                          guint                height);

/**
 * gegl_rectangle_equal:
 * @rectangle1: a #GeglRectangle
 * @rectangle2: a #GeglRectangle
 *
 * Check if two #GeglRectangles are equal.
 *
 * Returns TRUE if @rectangle and @rectangle2 are equal.
 */
gboolean    gegl_rectangle_equal         (const GeglRectangle *rectangle1,
                                          const GeglRectangle *rectangle2);

/**
 * gegl_rectangle_equal_coords:
 * @rectangle: a #GeglRectangle
 * @x: X coordinate
 * @y: Y coordinate
 * @width: width of rectangle
 * @height: height of rectangle
 *
 * Check if a rectangle is equal to a set of parameters.
 *
 * Returns TRUE if @rectangle and @x,@y @width x @height are equal.
 */
gboolean    gegl_rectangle_equal_coords  (const GeglRectangle *rectangle,
                                          gint                 x,
                                          gint                 y,
                                          gint                 width,
                                          gint                 height);

/**
 * gegl_rectangle_copy:
 * @destination: a #GeglRectangle
 * @source: a #GeglRectangle
 *
 * Copies the rectangle information stored in @source over the information in
 * @destination.
 */
void        gegl_rectangle_copy          (GeglRectangle       *destination,
                                          const GeglRectangle *source);

/**
 * gegl_rectangle_bounding_box:
 * @destination: a #GeglRectangle
 * @source1: a #GeglRectangle
 * @source2: a #GeglRectangle
 *
 * Computes the bounding box of the rectangles @source1 and @source2 and stores the
 * resulting bounding box in @destination.
 */
void        gegl_rectangle_bounding_box  (GeglRectangle       *destination,
                                          const GeglRectangle *source1,
                                          const GeglRectangle *source2);

/**
 * gegl_rectangle_intersect:
 * @dest: return location for the intersection of @src1 and @src2, or NULL.
 * @src1: a #GeglRectangle
 * @src2: a #GeglRectangle
 *
 * Calculates the intersection of two rectangles. It is allows for dest to be the same
 * as either @src1 or @src2. If the rectangles do not intersect, dest's width and height
 * are set to 0 and its x and y values are undefined.
 *
 * Returns TRUE if the rectangles intersect.
 */
gboolean    gegl_rectangle_intersect     (GeglRectangle       *dest,
                                          const GeglRectangle *src1,
                                          const GeglRectangle *src2);

/**
 * gegl_rectangle_contains:
 * @parent: a #GeglRectangle
 * @child: a #GeglRectangle
 *
 * Checks if the #GeglRectangle @child is fully contained within @parent.
 *
 * Returns TRUE if the @child is fully contained in @parent.
 */
gboolean    gegl_rectangle_contains      (const GeglRectangle *parent,
                                          const GeglRectangle *child);

/**
 * gegl_rectangle_infinite_plane:
 *
 * Returns a GeglRectangle that represents an infininte plane.
 */
GeglRectangle gegl_rectangle_infinite_plane (void);

/**
 * gegl_rectangle_is_infinite_plane:
 * @rectangle: A GeglRectangle.
 *
 * Returns TRUE if the GeglRectangle represents an infininte plane,
 * FALSE otherwise.
 */
gboolean gegl_rectangle_is_infinite_plane (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_dump:
 * @rectangle: A GeglRectangle.
 *
 * For debugging purposes, not stable API.
 */
void     gegl_rectangle_dump              (const GeglRectangle *rectangle);


/***
 * Aligned memory:
 *
 * GEGL provides functions to allocate and free buffers that are guaranteed to
 * be on 16 byte aligned memory addresses.
 */

/**
 * gegl_malloc:
 * @n_bytes: the number of bytes to allocte.
 *
 * Allocates @n_bytes of memory. If n_bytes is 0 it returns NULL.
 *
 * Returns a pointer to the allocated memory.
 */
gpointer gegl_malloc                  (gsize    n_bytes);

/**
 * gegl_free:
 * @mem: the memory to free.
 *
 * Frees the memory pointed to by @mem, if @mem is NULL it will warn and abort.
 */
void     gegl_free                    (gpointer mem);


#define GEGL_FLOAT_EPSILON            (1e-5)
#define GEGL_FLOAT_IS_ZERO(value)     (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2)      (_gegl_float_epsilon_equal ((v1), (v2)))

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/***
 */
inline gint _gegl_float_epsilon_zero  (float     value);
gint        _gegl_float_epsilon_equal (float     v1,
                                       float     v2);

G_END_DECLS

#endif /* __GEGL_UTILS_H__ */
