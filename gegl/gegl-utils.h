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
 * gegl_rectangle_new:
 * @x: upper left x coordinate
 * @y: upper left y coordinate
 * @width: width in pixels.
 * @height: height in pixels.
 *
 * Creates a new rectangle set with the values from @x, @y, @width and @height.
 */
GeglRectangle *gegl_rectangle_new        (gint                 x,
                                          gint                 y,
                                          guint                width,
                                          guint                height);

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
 * gegl_rectangle_is_empty:
 * @rectangle: a #GeglRectangle
 *
 * Check if a rectangle has zero area.
 *
 * Returns TRUE if @rectangle height and width are both zero.
 */
gboolean    gegl_rectangle_is_empty     (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_dup:
 * @rectangle: the #GeglRectangle to duplicate
 *
 * Create a new copy of @rectangle.
 *
 * Return value: (transfer full): a #GeglRectangle
 */
GeglRectangle *gegl_rectangle_dup       (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_copy:
 * @destination: a #GeglRectangle
 * @source: a #GeglRectangle
 *
 * Copies the rectangle information stored in @source over the information in
 * @destination.
 *
 * @destination may point to the same object as @source.
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
 *
 * @destination may point to the same object as @source1 or @source2.
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
 * Calculates the intersection of two rectangles. If the rectangles do not
 * intersect, dest's width and height are set to 0 and its x and y values
 * are undefined.
 *
 * @dest may point to the same object as @src1 or @src2.
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
 * gegl_malloc: (skip)
 * @n_bytes: the number of bytes to allocte.
 *
 * Allocates @n_bytes of memory. If n_bytes is 0 it returns NULL.
 *
 * Returns a pointer to the allocated memory.
 */
gpointer gegl_malloc                  (gsize    n_bytes) G_GNUC_MALLOC;

/**
 * gegl_free: (skip)
 * @mem: the memory to free.
 *
 * Frees the memory pointed to by @mem, if @mem is NULL it will warn and abort.
 */
void     gegl_free                    (gpointer mem);

/**
 * gegl_calloc: (skip)
 * @size: size of items to allocate
 * @n_memb: number of members
 *
 * allocated 0'd memory.
 */
gpointer gegl_calloc (gsize size, int n_memb) G_GNUC_MALLOC;

/**
 * gegl_memset_pattern: (skip)
 * @dst_ptr: pointer to copy to
 * @src_ptr: pointer to copy from
 * @pattern_size: the length of @src_ptr
 * @count: number of copies
 *
 * Fill @dst_ptr with @count copies of the bytes in @src_ptr.
 */
void gegl_memset_pattern              (void *       dst_ptr,
                                       const void * src_ptr,
                                       gint         pattern_size,
                                       gint         count);


#define GEGL_FLOAT_EPSILON            (1e-5)
#define GEGL_FLOAT_IS_ZERO(value)     (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2)      (_gegl_float_epsilon_equal ((v1), (v2)))

/**
 */
gint        _gegl_float_epsilon_zero  (float     value);
gint        _gegl_float_epsilon_equal (float     v1,
                                       float     v2);

typedef enum GeglSerializeFlag {
  GEGL_SERIALIZE_TRIM_DEFAULTS = (1<<0),
  GEGL_SERIALIZE_VERSION       = (1<<1),
  GEGL_SERIALIZE_INDENT        = (1<<2)
} GeglSerializeFlag;

/**
 * gegl_create_chain_argv:
 * @ops: an argv style, NULL terminated array of arguments
 * @op_start: node to pass in as input of chain
 * @op_end: node to get processed data
 * @time: the time to use for interpolatino of keyframed values
 * @rel_dim: relative dimension to scale rel suffixed values by
 * @path_root: path in filesystem to use as relative root
 * @error: error for signalling parsing errors
 *
 * Create a node chain from argv style list of op data.
 */
void gegl_create_chain_argv (char      **ops,
                             GeglNode   *op_start,
                             GeglNode   *op_end,
                             double      time,
                             int         rel_dim,
                             const char *path_root,
                             GError    **error);
/**
 * gegl_create_chain:
 * @ops: an argv style, NULL terminated array of arguments
 * @op_start: node to pass in as input of chain
 * @op_end: node to get processed data
 * @time: the time to use for interpolatino of keyframed values
 * @rel_dim: relative dimension to scale rel suffixed values by
 * @path_root: path in filesystem to use as relative root
 * @error: error for signalling parsing errors
 *
 * Create a node chain from an unparsed commandline string.
 */
void gegl_create_chain (const char *str,
                        GeglNode   *op_start,
                        GeglNode   *op_end,
                        double      time,
                        int         rel_dim,
                        const char *path_root,
                        GError    **error);

/**
 * gegl_serialize:
 * @start: first node in chain to serialize
 * @end: last node in chain to serialize
 * @basepath: top-level absolute path to turn into relative root
 * @serialize_flags: anded together combination of:
 * GEGL_SERIALIZE_TRIM_DEFAULTS, GEGL_SERIALIZE_VERSION, GEGL_SERIALIZE_INDENT.
 */

gchar *gegl_serialize  (GeglNode         *start,
                        GeglNode         *end,
                        const char       *basepath,
                        GeglSerializeFlag serialize_flags);

/* gegl_node_new_from_serialized:
 * @chaindata: string of chain serialized to parse.
 * @path_root: absolute file system root to use as root for relative paths.
 *
 * create a composition from chain serialization, creating end-points for
 * chain as/if needed.
 */
GeglNode *gegl_node_new_from_serialized (const gchar *chaindata,
                                         const gchar *path_root);

G_END_DECLS

#endif /* __GEGL_UTILS_H__ */
