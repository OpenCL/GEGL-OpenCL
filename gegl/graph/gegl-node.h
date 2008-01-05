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

#include "gegl-node-context.h"
#include <gegl/buffer/gegl-buffer.h>
#include <gegl/buffer/gegl-cache.h>

G_BEGIN_DECLS

#define GEGL_TYPE_NODE            (gegl_node_get_type ())
#define GEGL_NODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE, GeglNode))
#define GEGL_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE, GeglNodeClass))
#define GEGL_IS_NODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE))
#define GEGL_IS_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE))
#define GEGL_NODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE, GeglNodeClass))


typedef struct _GeglNodeClass   GeglNodeClass;
typedef struct _GeglNodePrivate GeglNodePrivate;

struct _GeglNode
{
  GObject         parent_instance;
  GeglOperation  *operation;
  GeglRectangle   have_rect;
  GSList         *pads;
  GSList         *input_pads;
  GSList         *output_pads;
  GSList         *sources;
  GSList         *sinks;

  gboolean        is_root;
  gboolean        enabled;

  GSList         *context;   /*< list of GeglNodeContext's corresponding to
                                 evaluation contexts */
  gboolean        is_graph;

  GeglCache      *cache;  /* For a node, the cache should be created at
                             first demand if applicable, and the cache object
                             reused for all subsequent requests for the cache
                             object.*/

  /*< private >*/
  GeglNodePrivate *priv;
};

typedef enum
{
  GEGL_BLIT_DEFAULT  = 0,
  GEGL_BLIT_CACHE    = 1 << 0,
  GEGL_BLIT_DIRTY    = 1 << 1,
} GeglBlitFlags;

enum
{
  GEGL_NODE_INVALIDATED,
  GEGL_NODE_COMPUTED,
  GEGL_NODE_LAST_SIGNAL
};

struct _GeglNodeClass
{
  GObjectClass parent_class;
};

/* renders the desired region of interest to a buffer of the specified
 * bablformat */
void          gegl_node_blit                (GeglNode      *node,
                                             gdouble        scale,
                                             GeglRectangle *roi,
                                             const Babl    *format,
                                             gpointer       destination_buf,
                                             gint           rowstride,
                                             GeglBlitFlags  flags);

void          gegl_node_link                (GeglNode      *source,
                                             GeglNode      *sink);

void          gegl_node_link_many           (GeglNode      *source,
                                             GeglNode      *dest,
                                             ...) G_GNUC_NULL_TERMINATED;

gboolean      gegl_node_connect_from        (GeglNode      *self,
                                             const gchar   *input_pad_name,
                                             GeglNode      *source,
                                             const gchar   *output_pad_name);

gboolean      gegl_node_connect_to          (GeglNode      *self,
                                             const gchar   *output_pad_name,
                                             GeglNode      *sink,
                                             const gchar   *input_pad_name);

gboolean      gegl_node_disconnect          (GeglNode      *self,
                                             const gchar   *input_pad_name);

void          gegl_node_set                 (GeglNode      *self,
                                             const gchar   *first_property_name,
                                             ...) G_GNUC_NULL_TERMINATED;
void          gegl_node_get                 (GeglNode      *self,
                                             const gchar   *first_property_name,
                                             ...) G_GNUC_NULL_TERMINATED;

/* functions below are internal to gegl */

GType         gegl_node_get_type            (void) G_GNUC_CONST;

GeglNodeContext *gegl_node_get_context      (GeglNode      *self,
                                             gpointer       context_id);
void             gegl_node_remove_context   (GeglNode      *self,
                                             gpointer       context_id);
GeglNodeContext *gegl_node_add_context      (GeglNode      *self,
                                             gpointer       context_id);

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
GeglNode    * gegl_node_get_producer        (GeglNode      *self,
                                             gchar         *pad_name,
                                             gchar        **output_pad);
GSList      * gegl_node_get_depends_on      (GeglNode      *self);
GeglBuffer  * gegl_node_apply               (GeglNode      *self,
                                             const gchar   *output_pad_name);
void          gegl_node_process             (GeglNode      *self);
void          gegl_node_set_valist          (GeglNode      *object,
                                             const gchar   *first_property_name,
                                             va_list        var_args);
void          gegl_node_get_valist          (GeglNode      *object,
                                             const gchar   *first_property_name,
                                             va_list        var_args);
void          gegl_node_set_property        (GeglNode      *object,
                                             const gchar   *property_name,
                                             const GValue  *value);
void          gegl_node_get_property        (GeglNode      *object,
                                             const gchar   *property_name,
                                             GValue        *value);
GParamSpec *  gegl_node_find_property       (GeglNode      *self,
                                             const gchar   *property_name);
void          gegl_node_set_need_rect       (GeglNode      *node,
                                             gpointer       context_id,
                                             gint           x,
                                             gint           y,
                                             gint           width,
                                             gint           height);


/* Graph related member functions of the GeglNode class */

GeglNode *    gegl_node_add_child           (GeglNode      *self,
                                             GeglNode      *child);
GeglNode *    gegl_node_remove_child        (GeglNode      *self,
                                             GeglNode      *child);
GeglNode *    gegl_node_get_nth_child       (GeglNode      *self,
                                             gint           n);
GSList   *    gegl_node_get_children        (GeglNode      *self);
void          gegl_node_remove_children     (GeglNode      *self);
gint          gegl_node_get_num_children    (GeglNode      *self);

GeglNode *    gegl_node_new                 (void);

GeglNode *    gegl_node_new_child           (GeglNode      *self,
                                             const gchar   *first_property_name,
                                             ...) G_GNUC_NULL_TERMINATED;
GeglNode *    gegl_node_create_child        (GeglNode      *self,
                                             const gchar   *operation);
GeglNode *    gegl_node_get_input_proxy     (GeglNode      *graph,
                                             const gchar   *name);
GeglNode *    gegl_node_get_output_proxy    (GeglNode      *graph,
                                             const gchar   *name);

const gchar * gegl_node_get_operation       (GeglNode      *node);

const gchar * gegl_node_get_debug_name      (GeglNode      *node);

GeglNode    * gegl_node_detect              (GeglNode      *root,
                                             gint           x,
                                             gint           y);
void          gegl_node_insert_before       (GeglNode      *self,
                                             GeglNode      *to_be_inserted);
gint          gegl_node_get_consumers       (GeglNode      *node,
                                             const gchar   *output_pad,
                                             GeglNode    ***nodes,
                                             const gchar ***pads);

GeglCache    *gegl_node_get_cache           (GeglNode      *node);
void          gegl_node_invalidated         (GeglNode      *node,
                                             GeglRectangle *rect);
GeglRectangle gegl_node_get_bounding_box    (GeglNode      *root);

const gchar  *gegl_node_get_name            (GeglNode      *self);
void          gegl_node_set_name            (GeglNode      *self,
                                             const gchar   *name);

extern guint gegl_node_signals[];

G_END_DECLS

#endif /* __GEGL_NODE_H__ */
