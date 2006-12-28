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
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_H__
#define __GEGL_H__

#include <glib-object.h>

#ifndef GEGL_INTERNAL

typedef struct _GeglNode      GeglNode;
typedef struct _GeglRectangle GeglRectangle;
typedef struct _GeglColor     GeglColor;

GType         gegl_node_get_type         (void) G_GNUC_CONST;
#define GEGL_TYPE_NODE (gegl_node_get_type())
#define GEGL_NODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))

GType         gegl_color_get_type        (void) G_GNUC_CONST;
#define GEGL_TYPE_COLOR (gegl_color_get_type())

/* NB: a GeglRectangle has the same internal structure as a GdkRectangle.*/
struct _GeglRectangle
{
  gint x;
  gint y;
  gint w;
  gint h;
};

#endif

/* Initialize the GEGL library, options are passed on to glib */
void          gegl_init                  (gint    *argc,
                                          gchar ***argv);
/* Clean up the gegl library after use (global caches etc.) */
void          gegl_exit                  (void);

/* Create a new gegl graph */
GeglNode    * gegl_graph_new             (void);

/* create a new node belonging to a graph */
GeglNode    * gegl_graph_create_node     (GeglNode     *graph,
                                          const gchar  *operation);

/* create a new node belonging to a graph, with key/value pairs for properties,
 * terminated by NULL (remember to set "operation") */
GeglNode    * gegl_graph_new_node        (GeglNode     *graph,
                                          const gchar  *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;
/* connect the output pad of a different node to this nodes input pad,
 * pads specified by names ("input","aux" and "output" are the names
 * currently in use
 */
gboolean      gegl_node_connect_from     (GeglNode     *sink,
                                          const gchar  *input_pad_name,
                                          GeglNode     *source,
                                          const gchar  *output_pad_name);

/* Connect the data coming from one of our output pads to an input pad
 * on an other node */
gboolean      gegl_node_connect_to          (GeglNode    *self,
                                             const gchar *output_pad_name,
                                             GeglNode    *sink,
                                             const gchar *input_pad_name);

/* included mainly for language bindings */
void          gegl_node_set_property        (GeglNode     *object,
                                             const gchar  *property_name,
                                             const GValue *value);
/* included mainly for language bindings */
void          gegl_node_get_property        (GeglNode     *object,
                                             const gchar  *property_name,
                                             GValue       *value);

/* Lookup the GParamSpec of an operations property, returns NULL
 * if the property was not found.
 */
GParamSpec *  gegl_node_find_property       (GeglNode     *self,
                                             const gchar  *property_name);

/* Break a connection.
 */
gboolean      gegl_node_disconnect       (GeglNode        *self,
                                          const gchar     *input_pad_name);

/* syntetic sugar for linking two nodes "output"->"input" */
void          gegl_node_link             (GeglNode        *source,
                                          GeglNode        *sink);

/* syntetic sugar for linking multiple nodes, end with NULL*/
void          gegl_node_link_many        (GeglNode        *source,
                                          GeglNode        *dest,
                                          ...) G_GNUC_NULL_TERMINATED;

/* set properties on the node, a NULL terminated key/value list, similar
 * to gobject
 */
void          gegl_node_set              (GeglNode        *node,
                                          const gchar     *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;
/* Get properties from a node, a NULL terminated key/value list, similar
 * to gobject.
 */
void          gegl_node_get              (GeglNode        *node,
                                          const gchar     *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/* Render the "output" buffer resulting from a node to an external buffer.
 * rowstride of 0 indicates default rowstride. You have to make sure the
 * buffer is large enough yourself.
 */
void          gegl_node_blit             (GeglNode        *node,
                                          GeglRectangle   *roi,
                                          void            *format,
                                          gint             rowstride,
                                          gpointer        *destination_buf);

/* Returns the bounding box of the defined data in a projection of node.
 */
GeglRectangle      gegl_node_get_bounding_box (GeglNode   *node);

/*
 * Causes a evaluation to happen
 */
void          gegl_node_process          (GeglNode        *sink_node);

/* Returns the name of the operation (also available as a property)
 */
const gchar * gegl_node_get_operation    (GeglNode        *node);

/* aquire the attached ghost output pad of a Graph node,
 * create it if it does not exist */
GeglNode    * gegl_graph_output          (GeglNode        *graph,
                                          const gchar     *name);
/* aquire the attached ghost input pad of a Graph node,
 * create it if it does not exist */
GeglNode    * gegl_graph_input           (GeglNode        *graph,
                                          const gchar     *name);

/* create a geglgraph from parsed XML data */
GeglNode    * gegl_xml_parse             (const gchar     *xmldata);

GParamSpec ** gegl_node_get_properties   (GeglNode        *self,
                                          guint           *n_properties);

/* return a list of available operation names */
GSList      * gegl_operation_list_operations (void);

/* Serialize a GEGL graph to XML, the resulting data must
 * be freed. */
gchar       * gegl_to_xml                (GeglNode        *gegl);

GeglColor   * gegl_color_new             (const gchar     *string);

void          gegl_color_get_rgba        (GeglColor       *self,
                                          gfloat          *r,
                                          gfloat          *g,
                                          gfloat          *b,
                                          gfloat          *a);

void          gegl_color_set_rgba        (GeglColor       *self,
                                          gfloat           r,
                                          gfloat           g,
                                          gfloat           b,
                                          gfloat           a);

GeglNode     *gegl_node_detect           (GeglNode        *root,
                                          gint             x,
                                          gint             y);


#endif  /* __GEGL_H__ */
