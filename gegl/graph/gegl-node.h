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
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

G_BEGIN_DECLS

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
 *                             "operation", "gegl:load",
 *                             "path",      "input.png",
 *                             NULL);
 * bcontrast = gegl_node_new_child (gegl,
 *                                  "operation", "gegl:brightness-contrast",
 *                                  "brightness", 0.2,
 *                                  "contrast",   1.5,
 *                                  NULL);
 */

/**
 * gegl_node_new:
 *
 * Create a new graph that can contain further processing nodes.
 *
 * Return value: (transfer full): A new top level #GeglNode (which can be used as a graph). When you
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
 * Return value: (transfer none): A newly created #GeglNode. The node will be destroyed by the parent.
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
 * gegl_node_blit: (skip)
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
 * gegl_node_blit_buffer:
 * @node: a #GeglNode
 * @buffer: (transfer none) (allow-none): the #GeglBuffer to render to.
 * @roi: (allow-none): the rectangle to render.
 * @level: mipmap level to render (0 for all)
 *
 * Render a rectangular region from a node to the given buffer.
 */
void          gegl_node_blit_buffer      (GeglNode            *node,
                                          GeglBuffer          *buffer,
                                          const GeglRectangle *roi,
                                          int                  level,
                                          GeglAbyssPolicy      abyss_policy);

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
 *                                 "operation", "gegl:png-save",
 *                                 "path",      "output.png",
 *                                 NULL);
 *
 * gegl_node_link (gegl, png_save);
 * gegl_node_process (png_save);
 *
 * buffer = malloc (roi.w*roi.h*4);
 * gegl_node_blit (gegl,
 *                 1.0,
 *                 &roi,
 *                 babl_format("R'G'B'A u8"),
 *                 buffer,
 *                 GEGL_AUTO_ROWSTRIDE,
 *                 GEGL_BLIT_DEFAULT);
 */
void          gegl_node_process          (GeglNode      *sink_node);


/***
 * Reparenting:
 *
 * Sometimes it is useful to be able to move nodes between graphs or even
 * handle orphaned nods that are not part of a graph. gegl_node_adopt_child
 * and gegl_node_get_parent are provided to handle such cases.
 */

/**
 * gegl_node_add_child:
 * @graph: a GeglNode (graph)
 * @child: a GeglNode.
 *
 * Make the GeglNode @graph, take a reference on child. This reference
 * will be dropped when the reference count on the graph reaches zero.
 *
 * Return value: (transfer none): the child.
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
 * Return value: (transfer none): the child.
 */
GeglNode *    gegl_node_remove_child        (GeglNode      *graph,
                                             GeglNode      *child);

/**
 * gegl_node_get_parent:
 * @node: a #GeglNode
 *
 * Returns a GeglNode that keeps a reference on a child.
 *
 * Return value: (transfer none): the parent of a node or NULL.
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
 * Return value: (transfer none): the GeglNode providing the
 * data ending up at @x,@y in the output of @node.
 */
GeglNode    * gegl_node_detect           (GeglNode      *node,
                                          gint           x,
                                          gint           y);


/**
 * gegl_node_find_property:
 * @node: the node to lookup a paramspec on
 * @property_name: the name of the property to get a paramspec for.
 *
 * Return value: (transfer none): the GParamSpec of property or NULL
 * if no such property exists.
 */
GParamSpec  * gegl_node_find_property    (GeglNode      *node,
                                          const gchar   *property_name);



/**
 * gegl_node_get_bounding_box: (skip)
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
 * Return value: (element-type Gegl.Node) (transfer container): a list
 * of the nodes contained within a GeglNode that is a subgraph.
 * Use g_list_free () on the list when done.
 */
GSList      * gegl_node_get_children     (GeglNode      *node);

/**
 * gegl_node_get_consumers:
 * @node: the node we are querying.
 * @output_pad: the output pad we want to know who uses.
 * @nodes: (out callee-allocates) (array zero-terminated=1) (allow-none): optional return location for array of nodes.
 * @pads: (out callee-allocates) (array zero-terminated=1) (allow-none): optional return location for array of pad names.
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
 * Return value: (transfer none): Returns an input proxy for the named pad.
 * If no input proxy exists with this name a new one will be created.
 */
GeglNode    * gegl_node_get_input_proxy  (GeglNode      *node,
                                          const gchar   *pad_name);

/**
 * gegl_node_get_operation:
 * @node: a #GeglNode
 *
 * Return value: The type of processing operation associated with this
 * node, or NULL if there is no op associated. The special name
 * "GraphNode" is returned if the node is the container of a subgraph.
 */
const gchar * gegl_node_get_operation    (const GeglNode *node);

/**
 * gegl_node_get_gegl_operation:
 * @node: a #GeglNode
 *
 * Return value: (transfer none) (allow-none): The operation object
 * assoicated with this node or NULL if there is no op associated.
 */
GeglOperation *gegl_node_get_gegl_operation   (GeglNode *node);

/**
 * gegl_node_get_output_proxy:
 * @node: a #GeglNode
 * @pad_name: the name of the pad.
 *
 * Proxies are used to route between nodes of a subgraph contained within
 * a node.
 *
 * Return value: (transfer none):  Returns a output proxy for the named pad.
 * If no output proxy exists with this name a new one will be created.
 */
GeglNode    * gegl_node_get_output_proxy (GeglNode      *node,
                                          const gchar   *pad_name);

/**
 * gegl_node_get_producer:
 * @node: the node we are querying
 * @input_pad_name: the input pad we want to get the producer for
 * @output_pad_name: (allow-none): optional pointer to a location where we can store a
 *                   freshly allocated string with the name of the output pad.
 *
 * Return value: (transfer none): the node providing data
 * or NULL if no node is connected to the input_pad.
 */
GeglNode    * gegl_node_get_producer     (GeglNode      *node,
                                          const gchar   *input_pad_name,
                                          gchar        **output_pad_name);

/**
 * gegl_node_has_pad:
 * @node: the node we are querying
 * @pad_name: the pad name we are looking for
 *
 * Returns TRUE if the node has a pad with the specified name
 */
gboolean      gegl_node_has_pad          (GeglNode      *node,
                                          const gchar   *pad_name);

/**
 * gegl_node_list_input_pads:
 * @node: the node we are querying
 *
 * If the node has any input pads this function returns a null terminated
 * array of pad names, otherwise it returns NULL. The return value can be
 * freed with g_strfreev().
 *
 * Return value: (transfer full) (array zero-terminated=1)
 */
gchar      ** gegl_node_list_input_pads  (GeglNode      *node);

/**
 * gegl_node_list_output_pads:
 * @node: the node we are querying
 *
 * If the node has any output pads this function returns a null terminated
 * array of pad names, otherwise it returns NULL. The return value can be
 * freed with g_strfreev().
 *
 * Return value: (transfer full) (array zero-terminated=1)
 */
gchar      ** gegl_node_list_output_pads (GeglNode      *node);

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
 * Return value: (transfer none):a newly created node. The node will be destroyed by the parent.
 * Calling g_object_unref on a node will cause the node to be dropped by the
 * parent. (You may also add additional references using
 * g_object_ref/g_object_unref, but in general relying on the parents reference
 * counting is easiest.)
 */

GeglNode     * gegl_node_create_child    (GeglNode      *parent,
                                          const gchar   *operation);


/**
 * gegl_node_get_property: (skip)
 * @node: the node to get a property from
 * @property_name: the name of the property to get
 * @value: (out): pointer to a GValue where the value of the property should be stored
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
 * @value: (in): a GValue containing the value to be set in the property.
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
 * Return value: (transfer full): a GeglNode containing the parsed XML as a subgraph.
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
 * Return value: (transfer full): a GeglNode containing the parsed XML as a subgraph.
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

gboolean       gegl_node_get_passthrough (GeglNode      *node);

void           gegl_node_set_passthrough (GeglNode      *node,
                                          gboolean       passthrough);


G_END_DECLS

#endif /* __GEGL_NODE_H__ */
