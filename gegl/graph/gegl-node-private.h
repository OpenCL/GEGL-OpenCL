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

#ifndef __GEGL_NODE_PRIVATE_H__
#define __GEGL_NODE_PRIVATE_H__

#include "gegl-cache.h"
#include "gegl-types-internal.h"
#include "gegl-node.h"

G_BEGIN_DECLS

#define GEGL_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE, GeglNodeClass))
#define GEGL_IS_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE))
#define GEGL_NODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE, GeglNodeClass))
/* The rest is in gegl-types.h */


typedef struct _GeglNodeClass   GeglNodeClass;
typedef struct _GeglNodePrivate GeglNodePrivate;

struct _GeglNode
{
  GObject         parent_instance;

  /* The current operation associated with this node */
  GeglOperation  *operation;

  /* The region for which this node provides pixel data */
  GeglRectangle   have_rect;

  /* If TRUE the above have_rect is correct and can be returned
   * directly instead of computed
   */
  gboolean        valid_have_rect;

  /* All the pads on this node, depends on operation */
  GSList         *pads;

  /* The input pads */
  GSList         *input_pads;

  /* The output pads */
  GSList         *output_pads;

  /* If a node is a graph it means it has children. Typically the
   * children connect to the input/output proxies of their graph
   * node. This results in that the graph node can more or less be
   * transparently treated as a normal node in most contexts
   */
  gboolean        is_graph;

  /* For a node, the cache should be created at first demand if
   * applicable, and the cache object reused for all subsequent
   * requests for the cache object.
   */
  GeglCache      *cache;

  /* Whether result is cached or not, inherited by children */
  gboolean        dont_cache;

  gboolean        use_opencl;

  GMutex          mutex;

  gint            passthrough;

  /*< private >*/
  GeglNodePrivate *priv;
};

struct _GeglNodeClass
{
  GObjectClass parent_class;
};

/* functions below are internal to gegl */

GType         gegl_node_get_type            (void) G_GNUC_CONST;

void          gegl_node_add_pad             (GeglNode      *self,
                                             GeglPad       *pad);
void          gegl_node_remove_pad          (GeglNode      *self,
                                             GeglPad       *pad);
GeglPad     * gegl_node_get_pad             (GeglNode      *self,
                                             const gchar   *name);
GSList      * gegl_node_get_pads            (GeglNode      *self);
GSList      * gegl_node_get_input_pads      (GeglNode      *self);
GSList      * gegl_node_get_sinks           (GeglNode      *self);
gint          gegl_node_get_num_sinks       (GeglNode      *self);

void          gegl_node_dump_depends_on     (GeglNode      *self);
void          gegl_node_set_property        (GeglNode      *object,
                                             const gchar   *property_name,
                                             const GValue  *value);

/* Graph related member functions of the GeglNode class */
GeglNode *    gegl_node_get_nth_child       (GeglNode      *self,
                                             gint           n);
void          gegl_node_remove_children     (GeglNode      *self);
gint          gegl_node_get_num_children    (GeglNode      *self);

const gchar * gegl_node_get_debug_name      (GeglNode      *node);

void          gegl_node_insert_before       (GeglNode      *self,
                                             GeglNode      *to_be_inserted);

GeglCache   * gegl_node_get_cache           (GeglNode      *node);
void          gegl_node_invalidated         (GeglNode      *node,
                                             const GeglRectangle *rect,
                                             gboolean             clean_cache);

const gchar * gegl_node_get_name            (GeglNode      *self);
void          gegl_node_set_name            (GeglNode      *self,
                                             const gchar   *name);
void
gegl_node_emit_computed (GeglNode *node,
                         const GeglRectangle *rect);


G_END_DECLS

#endif /* __GEGL_NODE_PRIVATE_H__ */
