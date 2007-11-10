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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_NODE_DYNAMIC_H__
#define __GEGL_NODE_DYNAMIC_H__

#include <gegl/buffer/gegl-buffer.h>

G_BEGIN_DECLS

#define GEGL_TYPE_NODE_DYNAMIC            (gegl_node_dynamic_get_type ())
#define GEGL_NODE_DYNAMIC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE_DYNAMIC, GeglNodeDynamic))
#define GEGL_NODE_DYNAMIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE_DYNAMIC, GeglNodeDynamicClass))
#define GEGL_IS_NODE_DYNAMIC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE_DYNAMIC))
#define GEGL_IS_NODE_DYNAMIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE_DYNAMIC))
#define GEGL_NODE_DYNAMIC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE_DYNAMIC, GeglNodeDynamicClass))

typedef struct _GeglNodeDynamicClass GeglNodeDynamicClass;

struct _GeglNodeDynamic
{
  GObject        parent_instance;
  GeglNode      *node;
  gpointer       context_id;

  GeglRectangle  need_rect;   /* the rectangle needed from this node */
  GeglRectangle  result_rect; /* the result computation rectangle for this node,
                                 (will differ if the needed rect extends beyond
                                 the defined rectangle, some operations might
                                 event force/suggest expansion of the result
                                 rect.. (contrast stretch?))
                               */
  gboolean       cached;       /* true if the cache can be used directly, and
                                  recomputation of inputs is unneccesary) */

  gint           refs;         /* set to number of nodes that depends on it
                                  before evaluation begins, each time data is
                                  fetched from the op the reference count is
                                  dropped, when it drops to zero, the op is
                                  asked to clean it's pads
                                */
  GSList        *property;      /* used internally for data being exchanged */
};

struct _GeglNodeDynamicClass
{
  GObjectClass   parent_class;
};

GType           gegl_node_dynamic_get_type         (void) G_GNUC_CONST;
GeglRectangle * gegl_node_dynamic_get_need_rect    (GeglNodeDynamic *node);
void            gegl_node_dynamic_set_need_rect    (GeglNodeDynamic *node,
                                                    gint             x,
                                                    gint             y,
                                                    gint             width,
                                                    gint             height);
GeglRectangle * gegl_node_dynamic_get_result_rect  (GeglNodeDynamic *node);
void            gegl_node_dynamic_set_result_rect  (GeglNodeDynamic *node,
                                                    gint             x,
                                                    gint             y,
                                                    gint             width,
                                                    gint             height);
void            gegl_node_dynamic_set_property     (GeglNodeDynamic *node,
                                                    const gchar     *name,
                                                    const GValue    *value);
void            gegl_node_dynamic_get_property     (GeglNodeDynamic *node,
                                                    const gchar     *name,
                                                    GValue          *value);
void            gegl_node_dynamic_remove_property  (GeglNodeDynamic *self,
                                                    const gchar     *name);

G_END_DECLS

#endif /* __GEGL_NODE_H__ */
