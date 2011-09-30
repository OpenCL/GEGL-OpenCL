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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 * Copyright 2007 Øyvind Kolås <pippin@gimp.org>
 */

/***
 * GeglPath:
 *
 * GeglPath is GEGLs means of storing the nodes and other knots and the
 * instructions for rendering 2d paths like poly lines, bezier curves and other
 * curve representations.
 */

#ifndef __GEGL_PATH_H__
#define __GEGL_PATH_H__

#include <glib-object.h>
#include <gegl-matrix.h>

G_BEGIN_DECLS

#define GEGL_TYPE_PATH            (gegl_path_get_type ())
#define GEGL_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PATH, GeglPath))
#define GEGL_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PATH, GeglPathClass))
#define GEGL_IS_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PATH))
#define GEGL_IS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PATH))
#define GEGL_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PATH, GeglPathClass))

typedef struct _GeglPathClass  GeglPathClass;

struct _GeglPath
{
  GObject parent_instance;
};

GType                gegl_path_get_type       (void) G_GNUC_CONST;

/* Internally the following structures are used, parts
 * of the internal implementation are exposed through
 * the path access API. The linked list api is currently
 * only used for adding new path interpolators/flatteners
 * with new knot interpretations.
 */

/***
 * GeglPathItem:
 *
 * A #GeglPathItem contains the type of instruction to perform as
 * well as it's arguments. In the public API the PathItem always has
 * 4 points internally only the needed amount of memory is stored
 * for a GeglPathItem.
 *
 *  </p><pre>typedef struct GeglPathPoint
 * {
 *   gfloat x;
 *   gfloat y;
 * } GeglPathPoint;</pre></p>
 * </p><pre>typedef struct GeglPathItem
 * {
 *   gchar  type;
 *   GeglPathPoint  point[4];
 * } GeglPathItem;</pre></p>
 *
 */


typedef struct GeglPathPoint
{
  gfloat x;
  gfloat y;
} GeglPathPoint;

typedef struct GeglPathItem
{
  gchar  type;     /* should perhaps be padded out? */
  GeglPathPoint  point[4]; /* Note: internally GeglPath operates with paths that
                    * have the exact number of pairs allocated.
                    */
} GeglPathItem;


/**
 * gegl_path_new:
 *
 * Creates a new #GeglPath with no nodes.
 *
 * Returns the newly created #GeglPath
 */
GeglPath           * gegl_path_new            (void);

/**
 * gegl_path_new_from_string:
 * @instructions: a string describing the path.
 *
 * Creates a new #GeglPath with the nodes described in the string
 * @instructions. See gegl_path_parse_string() for details of the
 * format of the string.
 *
 * Returns the newly created #GeglPath
 */
GeglPath           * gegl_path_new_from_string(const gchar *instructions);

/**
 * gegl_path_is_empty:
 * @path: a #GeglPath
 *
 * Check if the path contains any nodes.
 *
 * Returns TRUE if the path has no nodes.
 */
gboolean             gegl_path_is_empty       (GeglPath    *path);

/**
 * gegl_path_get_n_nodes:
 * @path: a #GeglPath
 *
 * Retrieves the number of nodes in the path.
 *
 * Return value: the number of nodes in the path.
 */
gint                 gegl_path_get_n_nodes    (GeglPath    *path);

/**
 * gegl_path_get_length:
 * @path: a #GeglPath
 *
 * Returns the total length of the path.
 *
 * Return value: the length of the path.
 */
gdouble              gegl_path_get_length     (GeglPath     *path);

/**
 * gegl_path_get_node:
 * @path: a #GeglPath
 * @index: the node number to retrieve
 * @node: (out): a pointer to a #GeglPathItem record to be written.
 *
 * Retrieve the node of the path at position @pos.
 *
 * Returns TRUE if the node was successfully retrieved.
 */
gboolean             gegl_path_get_node       (GeglPath     *path,
                                               gint          index,
                                               GeglPathItem *node);

/**
 * gegl_path_to_string:
 * @path: a #GeglPath
 *
 * Serialize the paths nodes to a string.
 *
 * Return value: return a string with instructions describing the string you
 * need to free this with g_free().
 */
gchar              * gegl_path_to_string      (GeglPath    *path);


/**
 * gegl_path_set_matrix:
 * @path: a #GeglPath
 * @matrix: (in) (transfer none): a #GeglMatrix3 to copy the matrix from
 *
 * Set the transformation matrix of the path.
 *
 * The path is transformed through this matrix when being evaluated,
 * causing the calculated positions and length to be changed by the transform.
 */
void                 gegl_path_set_matrix     (GeglPath    *path,
                                               GeglMatrix3 *matrix);

/**
 * gegl_path_get_matrix:
 * @path: a #GeglPath
 * @matrix: (out caller-allocates): a #GeglMatrix3 to copy the matrix into
 *
 * Get the transformation matrix of the path.
 */
void                 gegl_path_get_matrix     (GeglPath    *path,
                                               GeglMatrix3 *matrix);

/**
 * gegl_path_closest_point:
 * @path: a #GeglPath
 * @x: x coordinate.
 * @y: y coordinate
 * @on_path_x: return location for x coordinate on the path that was closest
 * @on_path_y: return location for y coordinate on the path that was closest
 * @node_pos_before: the node position interpreted before this position
 * was deemed the closest coordinate.
 *
 * Figure out what and where on a path is closest to arbitrary coordinates.
 *
 * Returns the length along the path where the closest point was encountered.
 */
gdouble              gegl_path_closest_point  (GeglPath     *path,
                                               gdouble       x,
                                               gdouble       y,
                                               gdouble      *on_path_x,
                                               gdouble      *on_path_y,
                                               gint         *node_pos_before);

/**
 * gegl_path_calc:
 * @path: a #GeglPath
 * @pos: how far along the path.
 * @x: return location for x coordinate.
 * @y: return location for y coordinate
 *
 * Compute the coordinates of the path at the @position (length measured from
 * start of path, not including discontinuities).
 */
gboolean             gegl_path_calc           (GeglPath     *path,
                                               gdouble       pos,
                                               gdouble      *x,
                                               gdouble      *y);

/**
 * gegl_path_calc_values:
 * @path: a #GeglPath
 * @num_samples: number of samples to compute
 * @xs: return location for x coordinates
 * @ys: return location for y coordinates
 *
 * Compute @num_samples for a path into the provided arrays @xs and @ys
 * the returned values include the start and end positions of the path.
 */
void                 gegl_path_calc_values    (GeglPath    *path,
                                               guint        num_samples,
                                               gdouble     *xs,
                                               gdouble     *ys);

/**
 * gegl_path_get_bounds:
 * @self: a #GeglPath.
 * @min_x: return location for minimum x coordinate
 * @max_x: return location for maximum x coordinate
 * @min_y: return location for minimum y coordinate
 * @max_y: return location for maximum y coordinate
 *
 * Compute the bounding box of a path.
 */
void                 gegl_path_get_bounds     (GeglPath     *self,
                                               gdouble      *min_x,
                                               gdouble      *max_x,
                                               gdouble      *min_y,
                                               gdouble      *max_y);

/* XXX: LP and RP are nasty hacks because the GEGL docs code scanner gets
 * confused, need to be fixed there
 */
#define LP (
#define RP )
typedef void LP *GeglNodeFunction RP LP const GeglPathItem *node,
                                      gpointer            user_data RP;
#undef LP
#undef RP

/**
 * gegl_path_foreach:
 * @path: a #GeglPath
 * @each_item: (closure user_data) (scope call): a function to call for each node in the path.
 * @user_data: user data to pass to the function (in addition to the GeglPathItem).
 *
 * Execute a provided function for every node in the path (useful for
 * drawing and otherwise traversing a path.)
 */
void                 gegl_path_foreach        (GeglPath        *path,
                                               GeglNodeFunction each_item,
                                               gpointer         user_data);

/**
 * gegl_path_foreach_flat:
 * @path: a #GeglPath
 * @each_item: (closure user_data) (scope call): a function to call for each node in the path.
 * @user_data: user data to pass to a node.
 *
 * Execute a provided function for the segments of a poly line approximating
 * the path.
 */
void                 gegl_path_foreach_flat   (GeglPath        *path,
                                               GeglNodeFunction each_item,
                                               gpointer         user_data);


/**
 * gegl_path_clear:
 * @path: a #GeglPath
 *
 * Remove all nods from a @path.
 */
void                 gegl_path_clear          (GeglPath    *path);

/**
 * gegl_path_insert_node:
 * @path: a #GeglPath
 * @pos: the position we want the new node to have.
 * @node: pointer to a structure describing the GeglPathItem we want to store
 *
 * Insert the new node @node at position @pos in @path.
 * if @pos = -1, the node is added in the last position.
 */
void                 gegl_path_insert_node    (GeglPath           *path,
                                               gint                pos,
                                               const GeglPathItem *node);
/**
 * gegl_path_replace_node:
 * @path: a #GeglPath
 * @pos: the position we want the new node to have.
 * @node: pointer to a structure describing the GeglPathItem we want to store.
 *
 * Replaces the exiting node at position @pos in @path.
 */
void                 gegl_path_replace_node   (GeglPath           *path,
                                               gint                pos,
                                               const GeglPathItem *node);
/**
 * gegl_path_remove_node:
 * @path: a #GeglPath
 * @pos: a node in the path.
 *
 * Removes the node number @pos in @path.
 */
void                 gegl_path_remove_node    (GeglPath    *path,
                                               gint         pos);

/**
 * gegl_path_parse_string:
 * @path: a #GeglPath
 * @instructions: a string describing a path.
 *
 * Parses @instructions and appends corresponding nodes to path (call
 * gegl_path_clean() first if you want to replace the existing path.
 */
void                 gegl_path_parse_string   (GeglPath    *path,
                                               const gchar *instructions);

/**
 * gegl_path_append:
 * @path: a #GeglPath
 * @...: first instruction.
 *
 * Use as follows: gegl_path_append (path, 'M', 0.0, 0.0);
 * and gegl_path_append (path, 'C', 10.0, 10.0, 50.0, 10.0, 60.0, 0.0) the
 * number of arguments are determined from the instruction provided.
 */
void                 gegl_path_append         (GeglPath    *path,
                                                            ...);

/**
 * gegl_path_freeze:
 * @path: a @GeglPath
 *
 * Make the @GeglPath stop firing signals as it changes must be paired with a
 * gegl_path_thaw() for the signals to start again.
 */
void                  gegl_path_freeze        (GeglPath *path);

/**
 * gegl_path_thaw:
 * @path: a @GeglPath
 *
 * Restart firing signals (unless the path has been frozen multiple times).
 */
void                  gegl_path_thaw          (GeglPath *path);

/***
 */
GParamSpec         *  gegl_param_spec_path    (const gchar *name,
                                               const gchar *nick,
                                               const gchar *blurb,
                                               GeglPath    *default_path,
                                               GParamFlags  flags);

#define GEGL_TYPE_PARAM_PATH    (gegl_param_path_get_type ())
#define GEGL_IS_PARAM_PATH(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PARAM_PATH))
GType                 gegl_param_path_get_type (void) G_GNUC_CONST;

/**
 * gegl_path_add_type:
 * @type: a gchar to recognize in path descriptions.
 * @items: the number of floating point data items the instruction takes
 * @description: a human readable description of this entry
 *
 * Adds a new type to the path system, FIXME this should probably
 * return something on registration conflicts, for now it expects
 * all registered paths to be aware of each other.
 */
void                  gegl_path_add_type      (gchar        type,
                                               gint         items,
                                               const gchar *description);

/***
 * GeglPathList: (skip)
 *  
 * Linked list used internally, and for the plug-in API for new path
 * interpolators.
 */
typedef struct GeglPathList
{
  struct GeglPathList *next;
  GeglPathItem         d;
} GeglPathList;


/**
 * gegl_path_list_append: (skip)
 * @head: a #GeglPathList
 * @...: additional #GeglPathList items to append
 *
 * Appends to path list, if head is NULL a new list is created
 */ 
GeglPathList *        gegl_path_list_append   (GeglPathList *head, ...);

/**
 * gegl_path_list_destroy: (skip)
 * @path: A #GeglPathList
 *
 * Frees up a path list
 */
GeglPathList *        gegl_path_list_destroy  (GeglPathList *path);


/***
 * GeglFlattenerFunc: (skip)
 *
 * prototype of function passed to gegl_path_add_flattener()
 */
typedef GeglPathList *(*GeglFlattenerFunc) (GeglPathList *original);

/** 
 * gegl_path_add_flattener: (skip)
 * @func: a #GeglFlattenerFunc
 *
 * Add a new flattener, the flattener should produce a type of path that
 * GeglPath already understands, if the flattener is unable to flatten
 * the incoming path (doesn't understand the instructions), the original
 * path should be returned.
 */
void                  gegl_path_add_flattener (GeglFlattenerFunc func);


/**
 * gegl_path_get_path: (skip)
 * @path: a #GeglPath
 *
 * Return the internal untouched #GeglPathList
 */
GeglPathList *        gegl_path_get_path (GeglPath *path);

/**
 * gegl_path_get_flat_path: (skip)
 * @path: a #GeglPath
 *
 * Return a polyline version of @path
 */
GeglPathList *        gegl_path_get_flat_path (GeglPath *path);

/***
 * GeglPathPoint: (skip)
 */

/**
 * gegl_path_point_lerp: (skip)
 * @dest: return location for the result
 * @a: origin GeglPathPoint
 * @b: destination GeglPathPoint
 * @t: ratio between @a and @b
 *
 * linear interpolation between two #GeglPathPoint
 */
void                  gegl_path_point_lerp    (GeglPathPoint    *dest,
                                               GeglPathPoint    *a,
                                               GeglPathPoint    *b,
                                               gfloat            t);

/**
 * gegl_path_point_dist: (skip)
 * @a: an arbitrary GeglPathPoint
 * @b: an arbitrary GeglPathPoint
 *
 * Compute the distance between #GeglPathPoint @a and @b
 */
gdouble               gegl_path_point_dist    (GeglPathPoint       *a,
                                               GeglPathPoint       *b);

G_END_DECLS

#endif /* __GEGL_PATH_H__ */
