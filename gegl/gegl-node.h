/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */

#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

#include "gegl-object.h"

G_BEGIN_DECLS


#define GEGL_TYPE_NODE            (gegl_node_get_type ())
#define GEGL_NODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
#define GEGL_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE, GeglNodeClass))
#define GEGL_IS_NODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE))
#define GEGL_IS_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE))
#define GEGL_NODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE, GeglNodeClass))


typedef struct _GeglNodeClass GeglNodeClass;

struct _GeglNode
{
  GeglObject     parent_instance;

  /*< private >*/
  
  GList         *properties;
  GList         *input_properties;
  GList         *output_properties;

  GList         *sources;
  GList         *sinks;

  gboolean       enabled;
};

struct _GeglNodeClass
{
  GeglObjectClass  parent_class;
};


GType          gegl_node_get_type              (void) G_GNUC_CONST;

void           gegl_node_add_property          (GeglNode     *self,
                                                GeglProperty *property);
void           gegl_node_remove_property       (GeglNode     *self,
                                                GeglProperty *property);
GeglProperty * gegl_node_get_property          (GeglNode     *self,
                                                const gchar  *name);
GList        * gegl_node_get_properties        (GeglNode     *self);
GList        * gegl_node_get_input_properties  (GeglNode     *self);
GList        * gegl_node_get_output_properties (GeglNode     *self);
gint           gegl_node_get_num_input_props   (GeglNode     *self);
gint           gegl_node_get_num_output_props  (GeglNode     *self);
GList        * gegl_node_get_sinks             (GeglNode     *self);
GList        * gegl_node_get_sources           (GeglNode     *self);
gint           gegl_node_get_num_sources       (GeglNode     *self);
gint           gegl_node_get_num_sinks         (GeglNode     *self);
gboolean       gegl_node_connect               (GeglNode     *sink,
                                                const gchar  *sink_prop_name,
                                                GeglNode     *source,
                                                const gchar  *source_prop_name);
gboolean       gegl_node_disconnect            (GeglNode     *sink,
                                                const gchar  *sink_prop_name,
                                                GeglNode     *source,
                                                const gchar  *source_prop_name);
void           gegl_node_disconnect_sinks      (GeglNode     *self);
void           gegl_node_disconnect_sources    (GeglNode     *self);
GList        * gegl_node_get_depends_on        (GeglNode     *self);
void           gegl_node_apply                 (GeglNode     *self,
                                                const gchar  *output_prop_name);


G_END_DECLS

#endif /* __GEGL_NODE_H__ */
