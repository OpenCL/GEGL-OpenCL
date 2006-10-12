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

typedef struct _GeglNode   GeglNode;
typedef struct _GeglRect   GeglRect;
typedef struct _GeglColor  GeglColor;

struct _GeglRect
{
  gint x;
  gint y;
  gint w;
  gint h;
};

#endif

/* Initialize the GEGL library, options are passed on to glib */
void       gegl_init              (gint    *argc,
                                   gchar ***argv);
/* Clean up the gegl library after use (global caches etc.) */
void       gegl_exit              (void);

/* Create a new gegl graph */
GeglNode * gegl_graph_new         (void);

/* create a new node belonging to a graph */
GeglNode * gegl_graph_create_node (GeglNode     *graph,
                                   const gchar  *operation);

/* create a new node belonging to a graph, with key/value pairs for properties,
 * terminated by NULL (remember to set "operation") */
GeglNode * gegl_graph_new_node    (GeglNode     *graph,
                                   const gchar  *first_property_name,
                                   ...);
/* connect the output pad of a different node to this nodes input pad,
 * pads specified by names ("input","aux" and "output" are the names
 * currently in use
 */
gboolean   gegl_node_connect      (GeglNode     *self,
                                   const gchar  *input_pad_name,
                                   GeglNode     *source,
                                   const gchar  *output_pad_name);

/* syntetic sugar for linking two nodes "output"->"input" */
void       gegl_node_link         (GeglNode     *source,
                                   GeglNode     *sink);

/* syntetic sugar for linking multiple nodes, end with NULL*/
void       gegl_node_link_many    (GeglNode     *source,
                                   GeglNode     *dest,
                                   ...);

/* set properties on the node, a NULL terminated key/value list, similar
 * to gobject
 */
void       gegl_node_set          (GeglNode     *self,
                                   const gchar  *first_property_name,
                                   ...);
/* Get properties from a node, a NULL terminated key/value list, similar
 * to gobject.
 */
void       gegl_node_get          (GeglNode     *self,
                                   const gchar  *first_property_name,
                                   ...);

/* Render the "output" buffer resulting from a node to an external buffer.
 * rowstride of 0 indicates default rowstride. You have to make sure the
 * buffer is large enough yourself.
 */
void       gegl_node_blit_buf     (GeglNode     *self,
                                   GeglRect     *roi,
                                   void         *format,
                                   gint          rowstride,
                                   gpointer     *destination_buf);


/*
 * Causes a evaluation to happen (this function will be deprecated)
 */
void       gegl_node_apply        (GeglNode     *self,
                                   const gchar  *output_pad_name);

/* aquire the attached ghost output pad of a Graph node,
 * create it if it does not exist */
GeglNode * gegl_graph_output      (GeglNode     *graph,
                                   const gchar  *name);
/* aquire the attached ghost input pad of a Graph node,
 * create it if it does not exist */
GeglNode * gegl_graph_input       (GeglNode     *graph,
                                   const gchar  *name);

/* create a geglgraph from parsed XML data */
GeglNode * gegl_xml_parse         (const gchar *xmldata);

/* Serialize a GEGL graph to XML, the resulting data must
 * be freed. */
gchar    * gegl_to_xml            (GeglNode *gegl);

GeglColor *  gegl_color_from_string            (const gchar *string);

#endif  /* __GEGL_H__ */
