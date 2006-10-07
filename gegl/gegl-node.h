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

#ifndef __GEGL_NODE_H__
#define __GEGL_NODE_H__

#include "gegl-graph.h"
#include <gegl/buffer/gegl-buffer.h>

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
  GeglGraph      parent_instance;

  /*< private >*/

  GeglOperation *operation;

  GeglRect       have_rect;
  GeglRect       need_rect;
  GeglRect       result_rect;

  GList         *pads;
  GList         *input_pads;
  GList         *output_pads;

  GList         *sources;
  GList         *sinks;

  gboolean       is_root;
  gboolean       is_graph;  /*< a node that is a graph, needs a bit special treatment */
  gint           refs;      /*< set to number of nodes that depends on it before evaluation begins,
                                each time data is fetched from the op the reference count is dropped,
                                when it drops to zero, the op is asked to clean it's pads
                             */
  gboolean       enabled;

  gboolean       dirty;
};

struct _GeglNodeClass
{
  GeglGraphClass  parent_class;
};

/* renders the desired region of interest to a buffer of the specified
 * bablformat */
void          gegl_node_blit_buf            (GeglNode     *self,
                                             GeglRect     *roi,
                                             void         *format,
                                             gint          rowstride,
                                             gpointer     *destination_buf);

void          gegl_node_link                (GeglNode     *source,
                                             GeglNode     *sink);

void          gegl_node_link_many           (GeglNode     *source,
                                             GeglNode     *dest,
                                             ...);

gboolean      gegl_node_connect             (GeglNode     *self,
                                             const gchar  *input_pad_name,
                                             GeglNode     *source,
                                             const gchar  *output_pad_name);

gboolean      gegl_node_disconnect          (GeglNode     *self,
                                             const gchar  *input_pad_name,
                                             GeglNode     *source,
                                             const gchar  *output_pad_name);

void          gegl_node_set                 (GeglNode     *self,
                                             const gchar  *first_property_name,
                                             ...);
void          gegl_node_get                 (GeglNode     *self,
                                             const gchar  *first_property_name,
                                             ...);

/* functions below are internal to gegl */

GType         gegl_node_get_type            (void) G_GNUC_CONST;
void          gegl_node_add_pad             (GeglNode     *self,
                                             GeglPad      *pad);
GeglPad     * gegl_node_create_pad          (GeglNode     *self,
                                             GParamSpec   *param_spec);
void          gegl_node_remove_pad          (GeglNode     *self,
                                             GeglPad      *pad);
GeglPad     * gegl_node_get_pad             (GeglNode     *self,
                                             const gchar  *name);
GList       * gegl_node_get_pads            (GeglNode     *self);
GList       * gegl_node_get_input_pads      (GeglNode     *self);
GList       * gegl_node_get_output_pads     (GeglNode     *self);
gint          gegl_node_get_num_input_pads  (GeglNode     *self);
gint          gegl_node_get_num_output_pads (GeglNode     *self);
GList       * gegl_node_get_sinks           (GeglNode     *self);
GList       * gegl_node_get_sources         (GeglNode     *self);
gint          gegl_node_get_num_sources     (GeglNode     *self);
gint          gegl_node_get_num_sinks       (GeglNode     *self);
void          gegl_node_disconnect_sinks    (GeglNode     *self);
void          gegl_node_disconnect_sources  (GeglNode     *self);


GeglNode    * gegl_node_get_connected_to    (GeglNode     *self,
                                             gchar        *pad_name);

GList       * gegl_node_get_depends_on      (GeglNode     *self);
void          gegl_node_apply               (GeglNode     *self,
                                             const gchar  *output_pad_name);
void          gegl_node_apply_roi           (GeglNode     *self,
                                             const gchar  *output_pad_name,
                                             GeglRect     *roi);
void          gegl_node_set_valist          (GeglNode     *object,
                                             const gchar  *first_property_name,
                                             va_list       var_args);
void          gegl_node_get_valist          (GeglNode     *object,
                                             const gchar  *first_property_name,
                                             va_list       var_args);
void          gegl_node_set_property        (GeglNode     *object,
                                             const gchar  *property_name,
                                             const GValue *value);
void          gegl_node_get_property        (GeglNode     *object,
                                             const gchar  *property_name,
                                             GValue       *value);
GParamSpec ** gegl_node_list_properties     (GeglNode     *self,
                                             guint        *n_properties);
GeglRect    * gegl_node_get_have_rect       (GeglNode     *node);
void          gegl_node_set_have_rect       (GeglNode     *node,
                                             gint          x,
                                             gint          y,
                                             gint          width,
                                             gint          height);
GeglRect    * gegl_node_get_need_rect       (GeglNode     *node);
void          gegl_node_set_need_rect       (GeglNode     *node,
                                             gint          x,
                                             gint          y,
                                             gint          width,
                                             gint          height);
GeglRect    * gegl_node_get_result_rect     (GeglNode     *node);
void          gegl_node_set_result_rect     (GeglNode     *node,
                                             gint          x,
                                             gint          y,
                                             gint          width,
                                             gint          height);

const gchar * gegl_node_get_op_type_name    (GeglNode     *node);

const gchar * gegl_node_get_debug_name      (GeglNode     *node);
G_END_DECLS

#endif /* __GEGL_NODE_H__ */
