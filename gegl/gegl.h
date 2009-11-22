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

#ifndef __GEGL_H__
#define __GEGL_H__

#include <glib-object.h>
#include <babl/babl.h>

#include <gegl-types.h>

#include <gegl-buffer.h>
#include <gegl-color.h>
#include <gegl-curve.h>
#include <gegl-path.h>
#include <gegl-version.h>


/***
 * The GEGL API:
 *
 * This document is both a tutorial and a reference for the C API of GEGL.
 * The concepts covered in this reference should also be applicable when
 * using other languages.
 *
 * The core API of GEGL isn't frozen yet and feedback regarding its use as
 * well as the clarity of this documentation is most welcome.
 */

G_BEGIN_DECLS

/***
 * Introduction:
 *
 * Algorithms created with GEGL are expressed as graphs of nodes. The nodes
 * have associated image processing operations. A node has output and input
 * pads which can be connected. By connecting these nodes in chains a set of
 * image operation filters and combinators can be applied to the image data.
 *
 * To make GEGL process data you request a rectangular region of a node's
 * output pad to be rendered into a provided linear buffer of any (supported
 * by babl) pixel format. GEGL uses information provided by the nodes to
 * determine the smallest buffers needed at each stage of processing.
 */

/***
 * Initialization:
 *
 * Before GEGL can be used the engine should be initialized by either calling
 * #gegl_init or through the use of #gegl_get_option_group. To shut down the
 * GEGL engine call #gegl_exit.
 *
 * ---Code sample:
 * #include <gegl.h>
 *
 * int main(int argc, char **argv)
 * {
 *   gegl_init (&argc, &argv);
 *       # other GEGL code
 *   gegl_exit ();
 * }
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
 * arguments. gegl_init() will call g_thread_init(), unless you, or
 * some other code already has initialized gthread.
 *
 * Note that there is an alternative way to initialize GEGL: if you
 * are calling g_option_context_parse() with the option group returned
 * by #gegl_get_option_group(), you don't have to call #gegl_init() but
 * you have to call g_thread_init() before any glib or glib dependant code
 * yourself.
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
 * Available operations:
 * Gegl provides means to check for available processing operations that
 * can be used with nodes using #gegl_list_operations and for a specified
 * op give a list of properties with #gegl_list_properties.
 */

/**
 * gegl_list_operations:
 * @n_operations_p: return location for number of operations.
 *
 * Returns an alphabetically sorted array of available operation names. The
 * list should be freed with g_free after use.
 * ---
 * gchar **operations;
 * guint   n_operations;
 * gint i;
 *
 * operations = gegl_list_operations (&n_operations);
 * g_print ("Available operations:\n");
 * for (i=0; i < n_operations; i++)
 *   {
 *     g_print ("\t%s\n", operations[i]);
 *   }
 * g_free (operations);
 */
gchar        **gegl_list_operations         (guint *n_operations_p);


/**
 * gegl_list_properties:
 * @operation_type: the name of the operation type we want to query to properties of.
 * @n_properties_p: return location for number of properties.
 *
 * Returns an allocated array of #GParamSpecs describing the properties
 * of the operation available when a node has operation_type set.
 */
GParamSpec** gegl_list_properties           (const gchar   *operation_type,
                                             guint         *n_properties_p);

/***
 * GeglNode:
 *
 * The Node is the image processing primitive connected to create compositions
 * in GEGL. The toplevel #GeglNode which contains a graph of #GeglNodes is
 * created with #gegl_node_new. Using this toplevel node we can create children
 * of this node which are individual processing elements using #gegl_node_new_child
 *
 * A node either has an associated operation or is a parent for other nodes,
 * that are connected to their parent through proxies created with
 * #gegl_node_get_input_proxy and #gegl_node_get_output_proxy.
 *
 * The properties available on a node depends on which <a
 * href='operations.html'>operation</a> is specified.
 *
 * ---
 * GeglNode *gegl, *load, *bcontrast;
 *
 * gegl = gegl_node_new ();
 * load = gegl_node_new_child (gegl,
 *                             "operation", "load",
 *                             "path",      "input.png",
 *                             NULL);
 * bcontrast = gegl_node_new_child (gegl,
 *                                  "operation", "brightness-contrast",
 *                                  "brightness", 0.2,
 *                                  "contrast",   1.5,
 *                                  NULL);
 */

/**
 * gegl_node_new:
 *
 * Create a new graph that can contain further processing nodes.
 *
 * Returns a new top level #GeglNode (which can be used as a graph). When you
 * are done using this graph instance it should be unreferenced with g_object_unref.
 * This will also free any sub nodes created from this node.
 */
GeglNode     * gegl_node_new             (void);

/**
 * gegl_node_new_child:
 * @parent: a #GeglNode
 * @first_property_name: the first property name
 * @...: first property value, optionally followed by more key/value pairs, ended
 * terminated with NULL.
 *
 * Creates a new processing node that performs the specified operation with
 * a NULL terminated list of key/value pairs for initial parameter values
 * configuring the operation. Usually the first pair should be "operation"
 * and the type of operation to be associated. If no operation is provided
 * the node doesn't have an initial operation and can be used to construct
 * a subgraph with special middle-man routing nodes created with
 * #gegl_node_get_output_proxy and #gegl_node_get_input_proxy.
 *
 * Returns a newly created node. The node will be destroyed by the parent.
 * Calling g_object_unref on a node will cause the node to be dropped by the
 * parent. (You may also add additional references using
 * g_object_ref/g_object_unref, but in general relying on the parents reference
 * counting is easiest.)
 */
GeglNode    * gegl_node_new_child        (GeglNode      *parent,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/***
 * Making connections:
 *
 * Nodes in GEGL are connected to each other. The resulting graph of nodes
 * represents the image processing pipeline to be processed.
 *
 * ---
 * gegl_node_link_many (background, over, png_save, NULL);
 * gegl_node_connect_to (translate, "output", over, "aux");
 * gegl_node_link_many (text, blur, translate, NULL);
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
 * Synthetic sugar for linking a chain of nodes with "input"->"output". The
 * list is NULL terminated.
 */
void          gegl_node_link_many        (GeglNode      *source,
                                          GeglNode      *first_sink,
                                          ...) G_GNUC_NULL_TERMINATED;

/**
 * gegl_node_disconnect:
 * @node: a #GeglNode
 * @input_pad: the input pad to disconnect.
 *
 * Disconnects node connected to @input_pad of @node (if any).
 *
 * Returns TRUE if a connection was broken.
 */
gboolean      gegl_node_disconnect       (GeglNode      *node,
                                          const gchar   *input_pad);

/***
 * Properties:
 *
 * Properties can be set either when creating the node with
 * #gegl_node_new_child as well as later when changing the initial
 * value with #gegl_node_set.
 *
 * To see what properties are available for a given operation look in the <a
 * href='operations.html'>Operations reference</a> or use
 * #gegl_node_get.
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
 * ---
 * gegl_node_set (node, "brightness", -0.2,
 *                      "contrast",   2.0,
 *                      NULL);
 */
void          gegl_node_set              (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/**
 * gegl_node_set_valist:
 * @node: a #GeglNode
 * @first_property_name: name of the first property to set
 * @args: value for the first property, followed optionally by more name/value
 * pairs, followed by NULL.
 *
 * valist version of #gegl_node_set
 */
void          gegl_node_set_valist       (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          va_list        args);

/**
 * gegl_node_get:
 * @node: a #GeglNode
 * @first_property_name: name of the first property to get.
 * @...: return location for the first property, followed optionally by more
 * name/value pairs, followed by NULL.
 *
 * Gets properties of a #GeglNode.
 * ---
 * double level;
 * char  *path;
 *
 * gegl_node_get (png_save, "path", &path, NULL);
 * gegl_node_get (threshold, "level", &level, NULL);
 */
void          gegl_node_get              (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          ...) G_GNUC_NULL_TERMINATED;

/**
 * gegl_node_get_valist:
 * @node: a #GeglNode
 * @first_property_name: name of the first property to get.
 * @args: return location for the first property, followed optionally by more
 * name/value pairs, followed by NULL.
 *
 * valist version of #gegl_node_get
 */
void          gegl_node_get_valist       (GeglNode      *node,
                                          const gchar   *first_property_name,
                                          va_list        args);


/***
 * Processing:
 *
 * There are two different ways to do processing with GEGL, either you
 * query any node providing output for a rectangular region to be rendered
 * using #gegl_node_blit, or you use #gegl_node_process on a sink node (A
 * display node, an image file writer or similar). To do iterative processing
 * you need to use a #GeglProcessor. See #gegl_processor_work for a code
 * sample.
 */

/**
 * gegl_node_blit:
 * @node: a #GeglNode
 * @scale: the scale to render at 1.0 is default, other values changes the
 * width/height of the sampled region.
 * @roi: the rectangle to render from the node, the coordinate system used is
 * coordinates after scale has been applied.
 * @format: the #BablFormat desired.
 * @destination_buf: a memory buffer large enough to contain the data, can be
 * left as NULL when forcing a rendering of a region.
 * @rowstride: rowstride in bytes, or GEGL_AUTO_ROWSTRIDE to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 * @flags: an or'ed combination of GEGL_BLIT_DEFAULT, GEGL_BLIT_CACHE and
 * GEGL_BLIT_DIRTY. if cache is enabled, a cache will be set up for subsequent
 * requests of image data from this node. By passing in GEGL_BLIT_DIRTY the
 * function will return with the latest rendered results in the cache without
 * regard to wheter the regions has been rendered or not.
 *
 * Render a rectangular region from a node.
 */
void          gegl_node_blit             (GeglNode            *node,
                                          gdouble              scale,
                                          const GeglRectangle *roi,
                                          const Babl          *format,
                                          gpointer             destination_buf,
                                          gint                 rowstride,
                                          GeglBlitFlags        flags);

/**
 * gegl_node_process:
 * @sink_node: a #GeglNode without outputs.
 *
 * Render a composition. This can be used for instance on a node with a "png-save"
 * operation to render all neccesary data, and make it be written to file. This
 * function wraps the usage of a GeglProcessor in a single blocking function
 * call. If you need a non-blocking operation, then make a direct use of
 * #gegl_processor_work. See #GeglProcessor.
 *
 * ---
 * GeglNode      *gegl;
 * GeglRectangle  roi;
 * GeglNode      *png_save;
 * unsigned char *buffer;
 *
 * gegl = gegl_parse_xml (xml_data);
 * roi      = gegl_node_get_bounding_box (gegl);
 * # create png_save from the graph, the parent/child relationship
 * # only mean anything when it comes to memory management.
 * png_save = gegl_node_new_child (gegl,
 *                                 "operation", "png-save",
 *                                 "path",      "output.png",
 *                                 NULL);
 *
 * gegl_node_link (gegl, png_save);
 * gegl_node_process (png_save);
 *
 * buffer = malloc (roi.w*roi.h*4);
 * gegl_node_blit (gegl,
 *                 &roi,
 *                 1.0,
 *                 babl_format("R'G'B'A u8",
 *                 roi.w*4,
 *                 buffer,
 *                 GEGL_BLIT_DEFAULT);
 */
void          gegl_node_process          (GeglNode      *sink_node);


/***
 * Reparenting:
 *
 * Sometimes it is useful to be able to move nodes between graphs or even
 * handle orphaned nods that are not part of a graph. gegl_node_adopt_child
 * and gegl_node_get_parent are provided to handle such cases.
 *
 * (gegl_node_adopt_child is deprecated, and will be removed in a future
 * release)
 */

/**
 * gegl_node_adopt_child:
 * @parent: a #GeglNode or NULL.
 * @child: a #GeglNode
 *
 * Adds @child to the responsibilities of @node, this makes the parent node
 * take a reference on the child that is kept as long as the parent itself is
 * being referenced. The node is stolen from an existing parent if there is one,
 * or a presumed existing reference is used. If @parent is NULL the child will
 * be orphaned and the developer is given a reference to be responsible of.
 *
 * Returns the child, or NULL if there was a problem with the arguments.
 */
GeglNode    * gegl_node_adopt_child        (GeglNode      *parent,
                                            GeglNode      *child);

/**
 * gegl_node_add_child:
 * @graph: a GeglNode (graph)
 * @child: a GeglNode.
 *
 * Make the GeglNode @graph, take a reference on child. This reference
 * will be dropped when the reference count on the graph reaches zero.
 *
 * Returns the child.
 */
GeglNode *    gegl_node_add_child           (GeglNode      *graph,
                                             GeglNode      *child);

/**
 * gegl_node_remove_child:
 * @graph: a GeglNode (graph)
 * @child: a GeglNode.
 *
 * Removes a child from a GeglNode. The reference previously held will be
 * dropped so increase the reference count before removing when reparenting
 * a child between two graphs.
 *
 * Returns the child.
 */
GeglNode *    gegl_node_remove_child        (GeglNode      *graph,
                                             GeglNode      *child);

/**
 * gegl_node_get_parent:
 * @node: a #GeglNode
 *
 * Returns a GeglNode that keeps a reference on a child.
 *
 * Returns the parent of a node or NULL.
 */
GeglNode    * gegl_node_get_parent         (GeglNode      *node);


/***
 * State queries:
 *
 * This section lists functions that retrieve information, mostly needed
 * for interacting with a graph in a GUI, not creating one from scratch.
 *
 * You can figure out what the bounding box of a node is with #gegl_node_get_bounding_box,
 * retrieve the values of named properties using #gegl_node_get.
 */



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
GSList      * gegl_node_get_children     (GeglNode      *node);

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
 * Returns the type of processing operation associated with this node, or
 * NULL if there is no op associated. The special name "GraphNode"
 * is returned if the node is the container of a subgraph.
 */
const gchar * gegl_node_get_operation    (const GeglNode *node);

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


/***
 * Binding conveniences:
 *
 * The following functions are mostly included to make it easier to create
 * language bindings for the nodes. The varargs versions will in most cases
 * lead to both more efficient and readable code from C.
 */

/**
 * gegl_node_create_child:
 * @parent: a #GeglNode
 * @operation: the type of node to create.
 *
 * Creates a new processing node that performs the specified operation.
 * All properties of the operation will have their default values. This
 * is included as an addiiton to #gegl_node_new_child in the public API to have
 * a non varargs entry point for bindings as well as sometimes simpler more
 * readable code.
 *
 * Returns a newly created node. The node will be destroyed by the parent.
 * Calling g_object_unref on a node will cause the node to be dropped by the
 * parent. (You may also add additional references using
 * g_object_ref/g_object_unref, but in general relying on the parents reference
 * counting is easiest.)
 */

GeglNode     * gegl_node_create_child    (GeglNode      *parent,
                                          const gchar   *operation);


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
 * XML:
 * The XML format used by GEGL is not stable and should not be relied on
 * for anything but testing purposes yet.
 */

/**
 * gegl_node_new_from_xml:
 * @xmldata: a \0 terminated string containing XML data to be parsed.
 * @path_root: a file system path that relative paths in the XML will be
 * resolved in relation to.
 *
 * The #GeglNode returned contains the graph described by the tree of stacks
 * in the XML document. The tree is connected to the "output" pad of the
 * returned node and thus can be used directly for processing.
 *
 * Returns a GeglNode containing the parsed XML as a subgraph.
 */
GeglNode    * gegl_node_new_from_xml     (const gchar   *xmldata,
                                          const gchar   *path_root);

/**
 * gegl_node_new_from_file:
 * @path: the path to a file on the local file system to be parsed.
 *
 * The #GeglNode returned contains the graph described by the tree of stacks
 * in the XML document. The tree is connected to the "output" pad of the
 * returned node and thus can be used directly for processing.
 *
 * Returns a GeglNode containing the parsed XML as a subgraph.
 */
GeglNode    * gegl_node_new_from_file    (const gchar   *path);

/**
 * gegl_node_to_xml:
 * @node: a #GeglNode
 * @path_root: filesystem path to construct relative paths from.
 *
 * Returns a freshly allocated \0 terminated string containing a XML
 * serialization of the composition produced by a node (and thus also
 * the nodes contributing data to the specified node). To export a
 * gegl graph, connect the internal output node to an output proxy (see
 * #gegl_node_get_output_proxy.) and use the proxy node as the basis
 * for the serialization.
 */
gchar       * gegl_node_to_xml           (GeglNode      *node,
                                          const gchar   *path_root);
/***
 * GeglProcessor:
 *
 * A #GeglProcessor, is a worker that can be used for background rendering
 * of regions in a node's cache. Or for processing a sink node. For most
 * non GUI tasks using #gegl_node_blit and #gegl_node_process directly
 * should be sufficient. See #gegl_processor_work for a code sample.
 *
 */

/**
 * gegl_node_new_processor:
 * @node: a #GeglNode
 * @rectangle: the #GeglRectangle to work on or NULL to work on all available
 * data.
 *
 * Returns a new #GeglProcessor.
 */
GeglProcessor *gegl_node_new_processor      (GeglNode            *node,
                                             const GeglRectangle *rectangle);

/**
 * gegl_processor_set_rectangle:
 * @processor: a #GeglProcessor
 * @rectangle: the new #GeglRectangle the processor shold work on or NULL
 * to make it work on all data in the buffer.
 *
 * Change the rectangle a #GeglProcessor is working on.
 */
void           gegl_processor_set_rectangle (GeglProcessor       *processor,
                                             const GeglRectangle *rectangle);


/**
 * gegl_processor_work:
 * @processor: a #GeglProcessor
 * @progress: a location to store the (estimated) percentage complete.
 *
 * Do an iteration of work for the processor.
 *
 * Returns TRUE if there is more work to be done.
 *
 * ---
 * GeglProcessor *processor = gegl_node_new_processor (node, &roi);
 * double         progress;
 *
 * while (gegl_processor_work (processor, &progress))
 *   g_warning ("%f%% complete", progress);
 * gegl_processor_destroy (processor);
 */
gboolean       gegl_processor_work          (GeglProcessor *processor,
                                             gdouble       *progress);


/**
 * gegl_processor_destroy:
 * @processor: a #GeglProcessor
 *
 * Frees up resources used by a processing handle.
 */
void           gegl_processor_destroy       (GeglProcessor *processor);


/***
 * GeglConfig:
 *
 * GEGL uses a singleton configuration object
 */

/**
 * gegl_config:
 *
 * Returns a GeglConfig object with properties that can be manipulated to control
 * GEGLs behavior. Properties available on the object are:
 *
 * "cache-size" "quality" and "swap", the two first is an integer denoting
 * number of bytes, the secons a double value between 0 and 1 and the last
 * the path of the directory to swap to (or "ram" to not use diskbased swap)
 */
GeglConfig      * gegl_config (void);


/**
 * gegl_node:
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
 * Returns a new Gegl node.
 */
GeglNode *gegl_node (const gchar *op_type,
                     const gchar *first_property_name,
                     ...);


/**
 * gegl_graph:
 * @node: the end result of a composition created with gegl_node()
 *
 * Creates a GeglNode containing a free floating graph constructed
 * using gegl_node(). The GeglGraph adopts all the passed in nodes
 * making it sufficient to unref the resulting graph.
 *
 * gegl_graph (gegl_node ("gegl:over", NULL,
 *                        gegl_node (..), gegl_node (..)));
 *
 * Returns a GeglNode graph.
 */
GeglNode *gegl_graph (GeglNode *node);


G_END_DECLS
#endif  /* __GEGL_H__ */
