/* This file is the public GEGL API
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
 * 2000-2007 © Calvin Williamson, Øyvind Kolås.
 */

#ifndef __GEGL_H__
#define __GEGL_H__

#include <glib-object.h>


/***
 * API Reference:
 * This is the GEGL API reference, it is generated from the declarations and
 * comments in <a href='gegl.h.html'>gegl.h</a> the headerfile that forms the
 * public API of GEGL. This file contains both the functions that are to be used
 * when using gegl from C, as well as the functions that are intended for use
 * by language bindings for languages like ruby and python.
 */

/***
 * Initialization:
 *
 * Before GEGL can be used the engine should be initialized by either calling
 * #gegl_init or through the use of #gegl_get_option_group. To shut down the
 * GEGL engine call #gegl_exit.
 */

/**
 * gegl_init:
 * @argc: a pointer to the number of command line arguments.
 * @argv: a pointer to the array of command line arguments.
 *
 * Call this function before using any other GEGL functions. It will
 * initialize everything needed to operate GEGL and parses some
 * standard command line options.  @argc and @argv are adjusted
 * accordingly so your own code will never see those standard
 * arguments.
 *
 * Note that there is an alternative way to initialize GEGL: if you
 * are calling g_option_context_parse() with the option group returned
 * by #gegl_get_option_group(), you don't have to call #gegl_init().
 **/
void           gegl_init                 (gint          *argc,
                                          gchar       ***argv);
/**
 * gegl_get_option_group:
 *
 * Returns a GOptionGroup for the commandline arguments recognized
 * by GEGL. You should add this group to your GOptionContext
 * with g_option_context_add_group() if you are using
 * g_option_context_parse() to parse your commandline arguments.
 */
GOptionGroup * gegl_get_option_group     (void);

/**
 * gegl_exit:
 *
 * Call this function when you're done using GEGL. It will clean up
 * caches and write/dump debug information if the correct debug flags
 * are set.
 */
void           gegl_exit                 (void);


/***
 * GeglNode:
 *
 * The Node is the image processing primitive connected to create compositions
 * in GEGL. The toplevel #GeglNode which contains a graph of #GeglNodes can be
 * created with #gegl_node_new, from such a graph node, further children can be
 * created (that also might have their own children) using #gegl_node_new_child
 * and #gegl_node_create_child.
 */
#ifndef GEGL_INTERNAL /* These declarations duplicate internal ones in GEGL */
typedef struct _GeglNode      GeglNode;
GType gegl_node_get_type  (void) G_GNUC_CONST;
#define GEGL_TYPE_NODE  (gegl_node_get_type())
#define GEGL_NODE(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
typedef struct _GeglRectangle GeglRectangle;
#endif

/**
 * gegl_node_new:
 *
 * Create a new graph that can contain further processing nodes.
 *
 * Returns a new top level #GeglNode (which can be used as a graph). When you
 * are done using this graph instance it should be unreferenced with g_object_unref.
 */
GeglNode     * gegl_node_new             (void);

/**
 * gegl_node_create_child:
 * @parent: a #GeglNode
 * @operation: the type of node to create.
 *
 * Creates a new processing node that performs the specified operation.
 *
 * Returns a newly created node.
 */

GeglNode     * gegl_node_create_child    (GeglNode      *parent,
                                          const gchar   *operation);

/**
 * gegl_node_new_child:
 * @parent: a #GeglNode
 * @first_property_name: the first property name, should usually be "operation"
 * @...: first property value, optionally followed by more key/value pairs, ended
 * terminated with NULL.
 *
 * Creates a new processing node that performs the specified operation with
 * a NULL terminated list of key/value pairs for initial parameter values
 * configuring the operation.
 *
 * Returns a newly created node.
 */
GeglNode    * gegl_node_new_child        (GeglNode      *parent,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/***
 * Making connections:
 *
 * To do anything useful #GeglNode s needs to be connected together in
 * a graph, this can be done through multiple functions, all of which
 * are variations of #gegl_node_connect_from internally.
 */

/**
 * gegl_node_connect_from:
 * @sink: the node we're connecting an input to
 * @input_pad_name: the name of the input pad we are connecting to
 * @source: the node producing data we want to connect.
 * @output_pad_name: the output pad we want to use on the source.
 *
 * Makes a connection between the pads of two nodes.
 *
 * Returns TRUE if the connection was succesfully made.
 */

gboolean      gegl_node_connect_from     (GeglNode      *sink,
                                          const gchar   *input_pad_name,
                                          GeglNode      *source,
                                          const gchar   *output_pad_name);

/**
 * gegl_node_connect_to:
 * @source: the node producing data we want to connect.
 * @output_pad_name: the output pad we want to use on the source.
 * @sink: the node we're connecting an input to
 * @input_pad_name: the name of the input pad we are connecting to
 *
 * Makes a connection between the pads of two nodes.
 *
 * Returns TRUE if the connection was succesfully made.
 */
gboolean      gegl_node_connect_to       (GeglNode      *source,
                                          const gchar   *output_pad_name,
                                          GeglNode      *sink,
                                          const gchar   *input_pad_name);

/**
 * gegl_node_disconnect:
 * @node: a #GeglNode
 * @input_pad_name: the input pad to disconnect.
 *
 * Disconnects a data source from a node. (Should this be deprecated and
 * connecting a NULL node be used instead?)
 *
 * Returns TRUE if a connection was broken.
 */
gboolean      gegl_node_disconnect       (GeglNode      *node,
                                          const gchar   *input_pad_name);

/**
 * gegl_node_link:
 * @source: the producer of data.
 * @sink: the consumer of data.
 *
 * Synthetic sugar for linking the "output" pad of @source to the "input"
 * pad of @sink.
 */
void          gegl_node_link             (GeglNode      *source,
                                          GeglNode      *sink);

/**
 * gegl_node_link_many:
 * @source: the producer of data.
 * @first_sink: the first consumer of data.
 * @...: NULL, or optionally more consumers followed by NULL.
 *
 * Synthetic sugar for linking a chain of nodes with "input"->"output", the
 * list is NULL terminated. Making it possible to do things like:
 * <pre>gegl_node_link_many (png_load, blur, scale, crop, png_save, NULL);</pre>
 */
void          gegl_node_link_many        (GeglNode      *source,
                                          GeglNode      *first_sink,
                                          ...) G_GNUC_NULL_TERMINATED;

/***
 * Setting properties:
 *
 * Properties can be set either when creating the node with
 * #gegl_node_new_child or multiple properties can be set with #gegl_node_set.
 * #gegl_node_set_property is provided to make it possible to implement
 * language bindings without using variable arguments.
 *
 * To see what operations are available for a given operation look in the <a
 * href='operations.html'>Operations reference</a> or use
 * #gegl_node_get_properties.
 */

/**
 * gegl_node_set:
 * @node: a #GeglNode
 * @first_property_name: name of the first property to set
 * @...: value for the first property, followed optionally by more name/value
 * pairs, followed by NULL.
 *
 * Set properties on a node, possible properties to be set are the properties
 * of the currently set operations as well as <em>"name"</em> and
 * <em>"operation"</em>. <em>"operation"</em> changes the current operations
 * set for the node, <em>"name"</em> doesn't have any role internally in
 * GEGL.
 */
void          gegl_node_set              (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;
/**
 * gegl_node_set_property:
 * @node: a #GeglNode
 * @property_name: the name of the property to set
 * @value: a GValue containing the value to be set in the property.
 *
 * This is mainly included for language bindings. Using #gegl_node_set is
 * more convenient when programming in C.
 */
void          gegl_node_set_property     (GeglNode      *node,
                                          const gchar   *property_name,
                                          const GValue  *value);


/***
 * Processing:
 *
 * There are two different ways to do processing with GEGL, either you
 * query any node providing output for a rectangular region to be rendered
 * using #gegl_node_blit, or you use #gegl_node_process on a sink node (A
 * display node, an image file writer or similar).
 */

/**
 * gegl_node_blit:
 * @node: a #GeglNode
 * @roi: the rectangle to render
 * @format: the #BablFormat desired.
 * @rowstride: rowstride in bytes (currently ignored)
 * @destination_buf: a memory buffer large enough to contain the data.
 *
 * Render a rectangular region from a node.
 */
void          gegl_node_blit             (GeglNode      *node,
                                          GeglRectangle *roi,
                                          void          *format,
                                          gint           rowstride,
                                          gpointer      *destination_buf);
/**
 * gegl_node_process:
 * @sink_node: a #GeglNode without outputs.
 *
 * Render a composition. XXX: this will be replaced with an API that allows
 * the processing to occur in smaller chunks.
 */
void          gegl_node_process          (GeglNode      *sink_node);


/***
 * State queries:
 *
 * This section lists functions that retrieve information, mostly needed
 * for interacting with a graph in a GUI, not creating one from scratch.
 */

/**
 * gegl_list_operations:
 *
 * Returns a list of available operations names. The list should not be freed.
 */
GSList      * gegl_list_operations (void);

/**
 * gegl_node_detect:
 * @node: a #GeglNode
 * @x: x coordinate
 * @y: y coordinate
 *
 * Performs hit detection by returning the node providing data at a given
 * coordinate pair. Currently operates only on bounding boxes and not
 * pixel data.
 *
 * Returns the GeglNode providing the data ending up at @x,@y in the output
 * of @node.
 */
GeglNode    * gegl_node_detect           (GeglNode      *node,
                                          gint           x,
                                          gint           y);


/**
 * gegl_node_find_property:
 * @node: the node to lookup a paramspec on
 * @property_name: the name of the property to get a paramspec for.
 *
 * Returns the GParamSpec of property or NULL if no such property exists.
 */
GParamSpec  * gegl_node_find_property    (GeglNode      *node,
                                          const gchar   *property_name);


/**
 * gegl_node_get:
 * @node: a #GeglNode
 * @first_property_name: name of the first property to get.
 * @...: return location for the first property, followed optionally by more
 * name/value pairs, followed by NULL.
 *
 * Gets properties of a #GeglNode.
 */
void          gegl_node_get              (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/**
 * gegl_node_get_bounding_box:
 * @node: a #GeglNode
 *
 * Returns the position and dimensions of a rectangle spanning the area
 * defined by a node.
 */
GeglRectangle gegl_node_get_bounding_box (GeglNode      *node);

/**
 * gegl_node_get_children:
 * @node: the node to retrieve the children of.
 *
 * Returns a list of nodes with children/internal nodes. The list must be
 * freed by the caller.
 */
GList       * gegl_node_get_children     (GeglNode      *node);

/**
 * gegl_node_get_consumers:
 * @node: the node we are querying.
 * @output_pad: the output pad we want to know who uses.
 * @nodes: optional return location for array of nodes.
 * @pads: optional return location for array of pad names.
 *
 * Retrieve which pads on which nodes are connected to a named output_pad,
 * and the number of connections. Both the location for the generated
 * nodes array and pads array can be left as NULL. If they are non NULL
 * both should be freed with g_free. The arrays are NULL terminated.
 *
 * Returns the number of consumers connected to this output_pad.
 */
gint          gegl_node_get_consumers    (GeglNode      *node,
                                          const gchar   *output_pad,
                                          GeglNode    ***nodes,
                                          const gchar ***pads);

/**
 * gegl_node_get_input_proxy:
 * @node: a #GeglNode
 * @pad_name: the name of the pad.
 *
 * Proxies are used to route between nodes of a subgraph contained within
 * a node.
 *
 * Returns an input proxy for the named pad. If no input proxy exists with
 * this name a new one will be created.
 */
GeglNode    * gegl_node_get_input_proxy  (GeglNode      *node,
                                          const gchar   *pad_name);

/**
 * gegl_node_get_operation:
 * @node: a #GeglNode
 *
 * Returns the type of processing operation associated with this node.
 */
const gchar * gegl_node_get_operation    (GeglNode      *node);

/**
 * gegl_node_get_output_proxy:
 * @node: a #GeglNode
 * @pad_name: the name of the pad.
 *
 * Proxies are used to route between nodes of a subgraph contained within
 * a node.
 *
 * Returns a output proxy for the named pad. If no output proxy exists with
 * this name a new one will be created.
 */
GeglNode    * gegl_node_get_output_proxy (GeglNode      *node,
                                          const gchar   *pad_name);

/**
 * gegl_node_get_producer:
 * @node: the node we are querying
 * @input_pad_name: the input pad we want to get the producer for
 * @output_pad_name: optional pointer to a location where we can store a
 *                   freshly allocated string with the name of the output pad.
 *
 * Returns the node providing data or NULL if no node is connected to the
 * input_pad.
 */
GeglNode    * gegl_node_get_producer     (GeglNode      *node,
                                          gchar         *input_pad_name,
                                          gchar        **output_pad_name);

/**
 * gegl_node_get_properties:
 * @node: a #GeglNode
 * @n_properties: return location for number of properties.
 *
 * Returns an allocated array of #GParamSpecs describing the properties
 * of the operation currently set for a node.
 */
GParamSpec ** gegl_node_get_properties   (GeglNode      *node,
                                          guint         *n_properties);

/**
 * gegl_node_get_property:
 * @node: the node to get a property from
 * @property_name: the name of the property to get
 * @value: pointer to a GValue where the value of the property should be stored
 *
 * This is mainly included for language bindings. Using #gegl_node_get is
 * more convenient when programming in C.
 *
 */
void          gegl_node_get_property     (GeglNode      *node,
                                          const gchar   *property_name,
                                          GValue        *value);





/***
 * XML:
 * The XML format used by GEGL is not stable and should not be relied on
 * for anything but testing purposes yet.
 */

/**
 * gegl_parse_xml:
 * @xmldata: a \0 terminated string containing XML data to be parsed.
 * @path_root: a file system path that relative paths in the XML will be
 * resolved in relation to.
 *
 * Returns a GeglNode containing the parsed XML as a subgraph.
 */
GeglNode    * gegl_parse_xml             (const gchar   *xmldata,
                                          const gchar   *path_root);

/**
 * gegl_to_xml:
 * @node: a #GeglNode
 * @path_root: filesystem path to construct relative paths from.
 *
 * Returns a freshly allocated \0 terminated string containing a XML
 * serialization of a nodes children.
 */
gchar       * gegl_to_xml                (GeglNode      *node,
                                          const gchar   *path_root);

#ifndef GEGL_INTERNAL

/***
 * GeglRectangle:
 *
 * GeglRectangles are used in #gegl_node_get_bounding_box and #gegl_node_blit
 * for specifying rectangles.
 *
 * <pre>struct GeglRectangle
 * {
 *   gint x;
 *   gint y;
 *   gint w;
 *   gint h;
 * };</pre>
 *
 */
struct _GeglRectangle
{
  gint x;
  gint y;
  gint w;
  gint h;
};

/***
 * GeglColor:
 *
 * GeglColor is used for properties that use a gegl color, use #gegl_color_new
 * with a NULL string to create a new blank one, gegl_colors are destroyed
 * with g_object_unref when they no longer are needed.
 *
 * The colors used by gegls are described in a format similar to CSS, the
 * textstring "rgb(1.0,1.0,1.0)" signifies opaque white and
 * "rgba(1.0,0.0,0.0,0.75)" is a 75% opaque red. 
 */
typedef struct _GeglColor     GeglColor;
GType gegl_color_get_type (void) G_GNUC_CONST;
#define GEGL_TYPE_COLOR (gegl_color_get_type())
#endif

/**
 * gegl_color_new:
 * @string: an CSS style color string.
 *
 * Returns a #GeglColor object suitable for use with #gegl_node_set.
 */
GeglColor   * gegl_color_new             (const gchar   *string);

/**
 * gegl_color_get_rgba:
 * @color: a #GeglColor
 * @r: return location for red component.
 * @g: return location for green component.
 * @b: return location for blue component.
 * @a: return location for alpha component.
 *
 * Retrieve RGB component values from a #GeglColor.
 */
void          gegl_color_get_rgba        (GeglColor     *color,
                                          gfloat        *r,
                                          gfloat        *g,
                                          gfloat        *b,
                                          gfloat        *a);

/**
 * gegl_color_set_rgba:
 * @color: a #GeglColor
 * @r: new value for red component
 * @g: new value for green component
 * @b: new value for blue component
 * @a: new value for alpha component
 *
 * Retrieve RGB component values from a GeglColor.
 */
void          gegl_color_set_rgba        (GeglColor     *color,
                                          gfloat         r,
                                          gfloat         g,
                                          gfloat         b,
                                          gfloat         a);

/*** this is just here to trick the parser.
 */
#include "gegl/gegl-paramspecs.h"

#endif  /* __GEGL_H__ */
