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

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-node.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-utils.h"
#include "gegl-visitable.h"
#include "gegl-config.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operations.h"
#include "operation/gegl-operation-meta.h"

#include "process/gegl-eval-mgr.h"
#include "process/gegl-have-visitor.h"
#include "process/gegl-prepare-visitor.h"
#include "process/gegl-finish-visitor.h"
#include "process/gegl-processor.h"

enum
{
  PROP_0,
  PROP_OP_CLASS,
  PROP_OPERATION,
  PROP_NAME,
  PROP_DONT_CACHE
};

enum
{
  INVALIDATED,
  COMPUTED,
  LAST_SIGNAL
};


struct _GeglNodePrivate
{
  GSList         *source_connections;
  GSList         *sink_connections;
  GSList         *children;  /*  used for children */
  GeglNode       *parent;
  gchar          *name;
  GeglProcessor  *processor;
  GHashTable     *contexts;
  GeglEvalMgr    *eval_mgr[GEGL_MAX_THREADS];
};


static guint gegl_node_signals[LAST_SIGNAL] = {0};


static void            gegl_node_class_init               (GeglNodeClass *klass);
static void            gegl_node_init                     (GeglNode      *self);
static void            gegl_node_finalize                 (GObject       *self_object);
static void            gegl_node_dispose                  (GObject       *self_object);
static void            gegl_node_local_set_property       (GObject       *gobject,
                                                           guint          prop_id,
                                                           const GValue  *value,
                                                           GParamSpec    *pspec);
static void            gegl_node_local_get_property       (GObject       *gobject,
                                                           guint          prop_id,
                                                           GValue        *value,
                                                           GParamSpec    *pspec);
static gboolean        gegl_node_pads_exist               (GeglNode      *sink,
                                                           const gchar   *sink_pad_name,
                                                           GeglNode      *source,
                                                           const gchar   *source_pad_name);
static GeglConnection *gegl_node_find_connection          (GeglNode      *sink,
                                                           GeglPad       *sink_pad);
static void            gegl_node_visitable_iface_init     (gpointer       ginterface,
                                                           gpointer       interface_data);
static void            gegl_node_visitable_accept         (GeglVisitable *visitable,
                                                           GeglVisitor   *visitor);
static GSList*         gegl_node_visitable_depends_on     (GeglVisitable *visitable);
static void            gegl_node_set_operation_object     (GeglNode      *self,
                                                           GeglOperation *operation);
static void            gegl_node_set_op_class             (GeglNode      *self,
                                                           const gchar   *op_class,
                                                           const gchar   *first_property,
                                                           va_list        var_args);
static void            gegl_node_disconnect_sinks         (GeglNode      *self);
static void            gegl_node_disconnect_sources       (GeglNode      *self);
static void            gegl_node_property_changed         (GObject       *gobject,
                                                           GParamSpec    *arg1,
                                                           gpointer       user_data);


G_DEFINE_TYPE_WITH_CODE (GeglNode, gegl_node, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEGL_TYPE_VISITABLE,
                                                gegl_node_visitable_iface_init))


static void
gegl_node_class_init (GeglNodeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GeglNodePrivate));

  gobject_class->finalize     = gegl_node_finalize;
  gobject_class->dispose      = gegl_node_dispose;
  gobject_class->set_property = gegl_node_local_set_property;
  gobject_class->get_property = gegl_node_local_get_property;

  g_object_class_install_property (gobject_class, PROP_OPERATION,
                                   g_param_spec_object ("gegl-operation",
                                                        "Operation Object",
                                                        "The associated GeglOperation instance",
                                                        GEGL_TYPE_OPERATION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_OP_CLASS,
                                   g_param_spec_string ("operation",
                                                        "Operation Type",
                                                        "The type of associated GeglOperation",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DONT_CACHE,
                                   g_param_spec_boolean ("dont-cache",
                                                         "Do not cache",
                                                        "Do not cache the result of this operation, the property is inherited by children created from a node.",
                                                        TRUE,
                                                        G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The name of the node",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  gegl_node_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);

  gegl_node_signals[COMPUTED] =
    g_signal_new ("computed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);
}

static void
gegl_node_init (GeglNode *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            GEGL_TYPE_NODE,
                                            GeglNodePrivate);

  self->priv->contexts = g_hash_table_new (NULL, NULL);

  self->pads           = NULL;
  self->input_pads     = NULL;
  self->output_pads    = NULL;
  self->operation      = NULL;
  self->is_graph       = FALSE;
  self->cache          = NULL;
  self->mutex          = g_mutex_new ();

}

static void
gegl_node_visitable_iface_init (gpointer ginterface,
                                gpointer interface_data)
{
  GeglVisitableClass *visitable_class = ginterface;

  visitable_class->accept         = gegl_node_visitable_accept;
  visitable_class->depends_on     = gegl_node_visitable_depends_on;
}

static void
gegl_node_dispose (GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  if (self->priv->parent != NULL)
    {
      GeglNode *parent = self->priv->parent;
      self->priv->parent = NULL;
      gegl_node_remove_child (parent, self);
    }

  gegl_node_remove_children (self);
  if (self->cache)
    {
      g_object_unref (self->cache);
      self->cache = NULL;
    }

  {
    gint i;
    for (i=0; i<GEGL_MAX_THREADS; i++)
      if (self->priv->eval_mgr[i])
        {
          g_object_unref (self->priv->eval_mgr[i]);
          self->priv->eval_mgr[i] = NULL;
        }
  }

  if (self->priv->processor)
    {
      g_object_unref (self->priv->processor);
      self->priv->processor = NULL;
    }
  G_OBJECT_CLASS (gegl_node_parent_class)->dispose (gobject);
}

static void
gegl_node_finalize (GObject *gobject)
{
  GeglNode *self = GEGL_NODE (gobject);

  gegl_node_disconnect_sources (self);
  gegl_node_disconnect_sinks (self);

  if (self->pads)
    {
      g_slist_foreach (self->pads, (GFunc) g_object_unref, NULL);
      g_slist_free (self->pads);
      self->pads = NULL;
    }

  g_slist_free (self->input_pads);
  g_slist_free (self->output_pads);

  if (self->operation)
    {
      g_object_unref (self->operation);
      self->operation = NULL;
    }

  if (self->priv->name)
    {
      g_free (self->priv->name);
    }
  g_hash_table_destroy (self->priv->contexts);
  g_mutex_free (self->mutex);

  G_OBJECT_CLASS (gegl_node_parent_class)->finalize (gobject);
}

static void
gegl_node_local_set_property (GObject      *gobject,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (property_id)
    {
      case PROP_NAME:
        gegl_node_set_name (node, g_value_get_string (value));
        break;

      case PROP_DONT_CACHE:
        node->dont_cache = g_value_get_boolean (value);
        break;

      case PROP_OP_CLASS:
        {
          va_list null; /* dummy to pass along, it's not used anyways since
                         * the preceding argument is NULL, gcc might warn about
                         * use of uninitialized variable.
                         */
#if defined(__GNUC__)
          memset(&null, 0, sizeof(null));
#endif
          gegl_node_set_op_class (node, g_value_get_string (value), NULL, null);
        }
        break;

      case PROP_OPERATION:
        gegl_node_set_operation_object (node, g_value_get_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_node_local_get_property (GObject    *gobject,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GeglNode *node = GEGL_NODE (gobject);

  switch (property_id)
    {
      case PROP_OP_CLASS:
        if (node->operation)
          g_value_set_string (value, GEGL_OPERATION_GET_CLASS (node->operation)->name);
        break;

      case PROP_DONT_CACHE:
        g_value_set_boolean (value, node->dont_cache);
        break;
      case PROP_NAME:
        g_value_set_string (value, gegl_node_get_name (node));
        break;

      case PROP_OPERATION:
        g_value_set_object (value, node->operation);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

/**
 * gegl_node_get_pad:
 * @self: a #GeglNode.
 * @name: property name.
 *
 * Get a property.
 *
 * Returns: A #GeglPad.
 **/
GeglPad *
gegl_node_get_pad (GeglNode    *self,
                   const gchar *name)
{
  GSList *list;

  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!self->pads)
    return NULL;

  for (list = self->pads; list; list = g_slist_next (list))
    {
      GeglPad *property = list->data;

      if (!strcmp (name, gegl_pad_get_name (property)))
        return property;
    }

  return NULL;
}

gboolean
gegl_node_has_pad (GeglNode      *self,
                   const gchar   *name)
{
  return gegl_node_get_pad (self, name) != NULL;
}

/**
 * gegl_node_get_pads:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglPad.
 **/
GSList *
gegl_node_get_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->pads;
}

/**
 * gegl_node_get_input_pads:
 * @self: a #GeglNode.
 *
 * Returns: A list of #GeglPad.
 **/
GSList *
gegl_node_get_input_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->input_pads;
}

void
gegl_node_add_pad (GeglNode *self,
                   GeglPad  *pad)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PAD (pad));

  if (gegl_node_get_pad (self, gegl_pad_get_name (pad)))
    return;

  self->pads = g_slist_prepend (self->pads, pad);

  if (gegl_pad_is_output (pad))
    self->output_pads = g_slist_prepend (self->output_pads, pad);

  if (gegl_pad_is_input (pad))
    self->input_pads = g_slist_prepend (self->input_pads, pad);
}

void
gegl_node_remove_pad (GeglNode *self,
                      GeglPad  *pad)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PAD (pad));

  self->pads = g_slist_remove (self->pads, pad);

  if (gegl_pad_is_output (pad))
    self->output_pads = g_slist_remove (self->output_pads, pad);

  if (gegl_pad_is_input (pad))
    self->input_pads = g_slist_remove (self->input_pads, pad);

  g_object_unref (pad);
}

static gboolean
gegl_node_pads_exist (GeglNode    *sink,
                      const gchar *sink_pad_name,
                      GeglNode    *source,
                      const gchar *source_pad_name)
{
  GeglPad *sink_pad;

  GeglPad *source_pad;

  if (sink)
    {
      g_assert (sink_pad_name);
      sink_pad = gegl_node_get_pad (sink, sink_pad_name);
      if (!sink_pad || !gegl_pad_is_input (sink_pad))
        {
          g_warning ("%s: Can't find sink property %s of %s", G_STRFUNC,
                     sink_pad_name, gegl_node_get_debug_name (sink));
          return FALSE;
        }
    }

  if (source)
    {
      g_assert (source_pad_name);
      source_pad = gegl_node_get_pad (source, source_pad_name);
      if (!source_pad || !gegl_pad_is_output (source_pad))
        {
          g_warning ("%s: Can't find source property %s of %s", G_STRFUNC,
                     source_pad_name, gegl_node_get_debug_name (source));
          return FALSE;
        }
    }

  return TRUE;
}

static GeglConnection *
gegl_node_find_connection (GeglNode *sink,
                           GeglPad  *sink_pad)
{
  GSList *list;

  g_return_val_if_fail (GEGL_IS_NODE (sink), NULL);

  for (list = sink->priv->source_connections; list; list = g_slist_next (list))
    {
      GeglConnection *connection = list->data;

      if (sink_pad == gegl_connection_get_sink_pad (connection))
        return connection;
    }

  return NULL;
}

gboolean
gegl_node_connect_to (GeglNode    *source,
                      const gchar *source_pad_name,
                      GeglNode    *sink,
                      const gchar *sink_pad_name)
{
  return gegl_node_connect_from (sink, sink_pad_name, source, source_pad_name);
}

void
gegl_node_invalidated (GeglNode            *node,
                       const GeglRectangle *rect,
                       gboolean             clear_cache)
{
  g_return_if_fail (GEGL_IS_NODE (node));

  if (!rect)
    rect = &node->have_rect;

  if (node->cache)
    {
      if (rect && clear_cache)
        gegl_buffer_clear (GEGL_BUFFER (node->cache), rect);
      gegl_cache_invalidate (node->cache, rect);
    }
  node->valid_have_rect = FALSE;

  g_signal_emit (node, gegl_node_signals[INVALIDATED], 0,
                 rect, NULL);
}

static void
gegl_node_source_invalidated (GeglNode            *source,
                              const GeglRectangle *rect,
                              gpointer             data)
{
  GeglPad       *destination_pad = GEGL_PAD (data);
  GeglNode      *destination     = gegl_pad_get_node (destination_pad);
  GeglRectangle  dirty_rect;

  GEGL_NOTE (GEGL_DEBUG_INVALIDATION, "%s.%s is dirtied from %s (%i,%i %i×%i)",
             gegl_node_get_debug_name (destination), gegl_pad_get_name (destination_pad),
             gegl_node_get_debug_name (source),
             rect->x, rect->y,
             rect->width, rect->height);

  if (destination->operation)
    {
      dirty_rect =
        gegl_operation_get_invalidated_by_change (destination->operation,
                                                  gegl_pad_get_name (destination_pad),
                                                  rect);
    }
  else
    {
      dirty_rect = *rect;
    }

  gegl_node_invalidated (destination, &dirty_rect, FALSE);
}

gboolean
gegl_node_connect_from (GeglNode    *sink,
                        const gchar *sink_pad_name,
                        GeglNode    *source,
                        const gchar *source_pad_name)
{
  GeglNode    *real_sink            = sink;
  GeglNode    *real_source          = source;
  const gchar *real_sink_pad_name   = sink_pad_name;
  const gchar *real_source_pad_name = source_pad_name;

  g_return_val_if_fail (GEGL_IS_NODE (sink), FALSE);
  g_return_val_if_fail (sink_pad_name != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (source), FALSE);
  g_return_val_if_fail (source_pad_name != NULL, FALSE);

  /* For graph nodes we implicitly use the proxy nodes */
  if (sink->is_graph)
    {
      real_sink = gegl_node_get_input_proxy (sink, sink_pad_name);

      /* The name of the input pad of proxynop input nodes is always
       * "input"
       */
      real_sink_pad_name = "input";
    }
  if (source->is_graph)
    {
      real_source = gegl_node_get_output_proxy (source, source_pad_name);

      /* The name of the output pad of proxynop output nodes is always
       * "output"
       */
      real_source_pad_name = "output";
    }

  {
    GeglPad *pad;
    GeglPad *other_pad = NULL;

    pad = gegl_node_get_pad (real_sink, real_sink_pad_name);
    if (pad)
      other_pad = gegl_pad_get_connected_to (pad);
    else
      {
        g_warning ("%s: Didn't find pad '%s' of '%s'",
                   G_STRFUNC, real_sink_pad_name, gegl_node_get_debug_name (real_sink));
      }

    if (other_pad)
      {
        gegl_node_disconnect (real_sink, real_sink_pad_name);
      }
  }
  if (gegl_node_pads_exist (real_sink, real_sink_pad_name, real_source, real_source_pad_name))
    {
      GeglPad        *sink_pad   = gegl_node_get_pad (real_sink, real_sink_pad_name);
      GeglPad        *source_pad = gegl_node_get_pad (real_source, real_source_pad_name);
      GeglConnection *connection = gegl_pad_connect (sink_pad,
                                                     source_pad);

      gegl_connection_set_sink_node (connection, real_sink);
      gegl_connection_set_source_node (connection, real_source);

      real_sink->priv->source_connections = g_slist_prepend (real_sink->priv->source_connections, connection);
      real_source->priv->sink_connections = g_slist_prepend (real_source->priv->sink_connections, connection);

      g_signal_connect (G_OBJECT (real_source), "invalidated",
                        G_CALLBACK (gegl_node_source_invalidated), sink_pad);

      gegl_node_property_changed (G_OBJECT (real_source->operation), NULL, real_source);

      return TRUE;
    }

  return FALSE;
}

gboolean
gegl_node_disconnect (GeglNode    *sink,
                      const gchar *sink_pad_name)
{
  GeglNode    *real_sink          = sink;
  const gchar *real_sink_pad_name = sink_pad_name;

  g_return_val_if_fail (GEGL_IS_NODE (sink), FALSE);
  g_return_val_if_fail (sink_pad_name != NULL, FALSE);

  /* For graph nodes we implicitly use the proxy nodes */
  if (sink->is_graph)
    {
      real_sink = gegl_node_get_input_proxy (sink, sink_pad_name);

      /* The name of the input pad of proxynop input nodes is always
       * "input"
       */
      real_sink_pad_name = "input";
    }

  if (gegl_node_pads_exist (real_sink, real_sink_pad_name, NULL, NULL))
    {
      GeglPad        *sink_pad   = gegl_node_get_pad (real_sink, real_sink_pad_name);
      GeglConnection *connection = gegl_node_find_connection (real_sink, sink_pad);
      GeglNode       *source;
      GeglPad        *source_pad;

      if (!connection)
        return FALSE;

      source_pad = gegl_connection_get_source_pad (connection);
      source     = gegl_connection_get_source_node (connection);

      gegl_node_source_invalidated (source, &source->have_rect, sink_pad);

      {
        /* disconnecting dirt propagation */
        gulong handler;

        handler = g_signal_handler_find (source, G_SIGNAL_MATCH_DATA,
                                         gegl_node_signals[INVALIDATED],
                                         0, NULL, NULL, sink_pad);
        if (handler)
          {
            g_signal_handler_disconnect (source, handler);
          }
      }

      gegl_pad_disconnect (sink_pad, source_pad, connection);

      real_sink->priv->source_connections = g_slist_remove (real_sink->priv->source_connections, connection);
      source->priv->sink_connections = g_slist_remove (source->priv->sink_connections, connection);

      gegl_connection_destroy (connection);


      return TRUE;
    }

  return FALSE;
}

static void
gegl_node_disconnect_sources (GeglNode *self)
{
  while (TRUE)
    {
      GeglConnection *connection = g_slist_nth_data (self->priv->source_connections, 0);

      if (connection)
        {
          GeglNode    *sink          = gegl_connection_get_sink_node (connection);
          GeglPad     *sink_pad      = gegl_connection_get_sink_pad (connection);
          const gchar *sink_pad_name = gegl_pad_get_name (sink_pad);

          g_assert (self == sink);

          gegl_node_disconnect (sink, sink_pad_name);
        }
      else
        break;
    }
}

static void
gegl_node_disconnect_sinks (GeglNode *self)
{
  while (TRUE)
    {
      GeglConnection *connection = g_slist_nth_data (self->priv->sink_connections, 0);

      if (connection)
        {
          GeglNode    *sink          = gegl_connection_get_sink_node (connection);
          GeglNode    *source        = gegl_connection_get_source_node (connection);
          GeglPad     *sink_pad      = gegl_connection_get_sink_pad (connection);
          const gchar *sink_pad_name = gegl_pad_get_name (sink_pad);

          g_assert (self == source);

          gegl_node_disconnect (sink, sink_pad_name);
        }
      else
        break;
    }
}

/**
 * gegl_node_num_sinks:
 * @self: a #GeglNode.
 *
 * Gets the number of sinks
 *
 * Returns: number of sinks
 **/
gint
gegl_node_get_num_sinks (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_slist_length (self->priv->sink_connections);
}

/**
 * gegl_node_get_sinks:
 * @self: a #GeglNode.
 *
 * Gets list of sink connections attached to this self.
 *
 * Returns: list of sink connections.
 **/
GSList *
gegl_node_get_sinks (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), FALSE);

  return self->priv->sink_connections;
}

void
gegl_node_link (GeglNode *source,
                GeglNode *sink)
{
  g_return_if_fail (GEGL_IS_NODE (source));
  g_return_if_fail (GEGL_IS_NODE (sink));

  /* using connect_to is more natural here, but leads to an extra
   * function call, perhaps connect_to and connect_from should be swapped?
   */
  gegl_node_connect_to (source, "output", sink, "input");
}

void
gegl_node_link_many (GeglNode *source,
                     GeglNode *dest,
                     ...)
{
  va_list var_args;

  g_return_if_fail (GEGL_IS_NODE (source));
  g_return_if_fail (GEGL_IS_NODE (dest));

  va_start (var_args, dest);
  while (dest)
    {
      gegl_node_link (source, dest);
      source = dest;
      dest   = va_arg (var_args, GeglNode *);
    }
  va_end (var_args);
}

static void gegl_node_ensure_eval_mgr (GeglNode    *self,
                                       const gchar *pad,
                                       gint         no)
{
  if (!self->priv->eval_mgr[no])
    self->priv->eval_mgr[no] = gegl_eval_mgr_new (self, pad);
}

/* Will set the eval_mgr's roi to the supplied roi if defined, otherwise
 * it will use the node's bounding box. Then the gegl_eval_mgr_apply will
 * be called.
 */
static GeglBuffer *
gegl_node_apply_roi (GeglNode            *self,
                     const gchar         *output_pad_name,
                     const GeglRectangle *roi,
                     gint                 tid)
{
  /* This is a potential spot to multiplex paralell processing,
   * doing so, might cause a lot of tile overlap between
   * processes if were not careful (wouldn't neccesarily be totally
   * bad if that happens though.
   */
  GeglBuffer *buffer;

  /*g_print ("%i %i %i %i %i\n", tid, roi->x, roi->y, roi->width, roi->height);*/

  if (roi)
    {
      self->priv->eval_mgr[tid]->roi = *roi;
    }
  else
    {
      self->priv->eval_mgr[tid]->roi = gegl_node_get_bounding_box (self);
    }
  buffer = gegl_eval_mgr_apply (self->priv->eval_mgr[tid]);
  return buffer;
}


typedef struct ThreadData
{
  GeglNode      *node;
  gint           tid;
  GeglRectangle  roi;
  const gchar   *pad;

  const Babl          *format;
  gpointer             destination_buf;
  gint                 rowstride;
  GeglBlitFlags        flags;
} ThreadData;

static GThreadPool *pool = NULL;
static GMutex *mutex = NULL;
static GCond  *cond = NULL;
static gint    remaining_tasks = 0;

static void spawnrender (gpointer data,
                         gpointer foo)
{
  ThreadData *td = data;
  GeglBuffer * buffer;
  buffer = gegl_node_apply_roi (td->node, td->pad, &td->roi, td->tid);

  if ((buffer ) && td->destination_buf)
    {
      gegl_buffer_get (buffer, &td->roi, 1.0, td->format, td->destination_buf, td->rowstride,
                       GEGL_ABYSS_NONE);
    }

  /* and unrefing to ultimately clean it off from the graph */
  if (buffer)
    g_object_unref (buffer);

  g_mutex_lock (mutex);
  remaining_tasks --;
  if (remaining_tasks == 0)
    {
      /* we were the last task, we're not busy rendering any more */
      g_cond_signal (cond);
    }
  g_mutex_unlock (mutex);
}


void
gegl_node_blit (GeglNode            *self,
                gdouble              scale,
                const GeglRectangle *roi,
                const Babl          *format,
                gpointer             destination_buf,
                gint                 rowstride,
                GeglBlitFlags        flags)
{
  gint threads;
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (roi != NULL);

  threads = gegl_config ()->threads;
  if (threads > GEGL_MAX_THREADS)
    threads = 1;

  if (pool == NULL)
    {
      pool = g_thread_pool_new (spawnrender, NULL, threads, TRUE, NULL);
      mutex = g_mutex_new ();
      cond = g_cond_new ();
    }

  if (flags == GEGL_BLIT_DEFAULT)
#if 1  /* multi threaded version */
    {
      ThreadData data[GEGL_MAX_THREADS];
      gint i;

      /* Subdivide along the largest of width/height, this should be further
       * extended similar to the subdivizion done in GeglProcessor, to get as
       * square as possible subregions.
       */
      gboolean horizontal = roi->width > roi->height;

      gint rowskip = 0;

      if (!format)
        format = babl_format ("RGBA float"); /* XXX: This probably duplicates
                                                another hardcoded format, they
                                                should be turned into a
                                                constant. */
      if (horizontal)
        rowskip = (roi->width/threads) * babl_format_get_bytes_per_pixel (format);

      if (rowstride == GEGL_AUTO_ROWSTRIDE)
        rowstride = roi->width * babl_format_get_bytes_per_pixel (format);

      data[0].node = self;
      data[0].pad = "output";
      data[0].format = format;
      data[0].destination_buf = destination_buf;
      data[0].rowstride = rowstride;
      data[0].flags = flags;

      for (i=0;i<threads;i++)
        {
          data[i] = data[0];
          data[i].roi = *roi;
          gegl_node_ensure_eval_mgr (self, "output", i);
          if (horizontal)
            {
              data[i].roi.width = roi->width / threads;
              data[i].roi.x = roi->x + roi->width/threads * i;
            }
          else
            {
              data[i].roi.height = roi->height / threads;
              data[i].roi.y = roi->y + roi->height/threads * i;
            }

          data[i].tid = i;
          if (horizontal)
            data[i].destination_buf = ((gchar*)destination_buf + rowskip * i);
          else
            data[i].destination_buf = ((gchar*)destination_buf + rowstride * (roi->height/threads) * i);
        }
      if (horizontal)
        data[threads-1].roi.width = roi->width - (roi->width / threads)*(threads-1);
      else
        data[threads-1].roi.height = roi->height - (roi->height / threads)*(threads-1);

      remaining_tasks+=threads;

      if (threads==1)
        {
          for (i=0; i<threads; i++)
              spawnrender (&data[i], NULL);
        }
      else
        {
          for (i=0; i<threads-1; i++)
            g_thread_pool_push (pool, &data[i], NULL);
          spawnrender (&data[threads-1], NULL);

          g_mutex_lock (mutex);
          while (remaining_tasks!=0)
            g_cond_wait (cond, mutex);
          g_mutex_unlock (mutex);
        }
    }
#else /* thread free version, could be removed, left behind in case it 
         is needed for debugging
       */
    {
      GeglBuffer *buffer;

      gegl_node_ensure_eval_mgr (self, "output", 0);
      buffer = gegl_node_apply_roi (self, "output", roi, 0);
      if (buffer && destination_buf)
        {
          if (destination_buf)
            {
              gegl_buffer_get (buffer, 1.0, roi, format, destination_buf, rowstride);
            }

          if (scale != 1.0)
            {
              g_warning ("Scale %f!=1.0 in blit without cache NYI", scale);
            }
        }

      /* and unrefing to ultimately clean it off from the graph */
      if (buffer)
        g_object_unref (buffer);
    }
#endif
  else
    if ((flags & GEGL_BLIT_CACHE))
    {
      GeglCache *cache = gegl_node_get_cache (self);
      if (!(flags & GEGL_BLIT_DIRTY))
        {
          if (!self->priv->processor)
           self->priv->processor = gegl_node_new_processor (self, roi);

          gegl_processor_set_rectangle (self->priv->processor, roi);
          while (gegl_processor_work (self->priv->processor, NULL));
        }
      if (destination_buf && cache)
        {
          gegl_buffer_get (GEGL_BUFFER (cache), roi, scale,
                           format, destination_buf, rowstride,
                           GEGL_ABYSS_NONE);
        }
    }
}

static GSList *
gegl_node_get_depends_on (GeglNode *self)
{
  GSList *depends_on = NULL;
  GSList *llink;

  for (llink = self->priv->source_connections; llink; llink = g_slist_next (llink))
    {
      GeglConnection *connection = llink->data;
      GeglNode       *source_node;

      source_node = gegl_connection_get_source_node (connection);

      depends_on = g_slist_prepend (depends_on, source_node);
    }

  return depends_on;
}

void
gegl_node_dump_depends_on (GeglNode *self)
{
  GSList *depends_on = gegl_node_get_depends_on (self);
  GSList *iter       = NULL;

  g_print ("GeglNode %p depends on:\n", self);

  for (iter = depends_on; iter; iter = iter->next)
    {
      GeglNode *source_node = depends_on->data;
      g_print ("  %s\n", gegl_node_get_debug_name (source_node));
    }

  g_slist_free (depends_on);
}

static void
gegl_node_visitable_accept (GeglVisitable *visitable,
                            GeglVisitor   *visitor)
{
  gegl_visitor_visit_node (visitor, (GeglNode *) visitable);
}

static GSList *
gegl_node_visitable_depends_on (GeglVisitable *visitable)
{
  GeglNode *self = GEGL_NODE (visitable);

  return gegl_node_get_depends_on (self);
}

static void
gegl_node_set_op_class (GeglNode    *node,
                        const gchar *op_class,
                        const gchar *first_property,
                        va_list      var_args)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (op_class);

  if (op_class && op_class[0] != '\0')
    {
      GType          type;
      GeglOperation *operation;

      type = gegl_operation_gtype_from_name (op_class);

      if (!type)
        {
          g_warning ("Failed to set operation type %s, using a passthrough op instead", op_class);
          if (strcmp (op_class, "gegl:nop"))
            {
              gegl_node_set_op_class (node, "gegl:nop", NULL, var_args);
            }
          else
            {
              g_warning ("The failing op was 'gegl:nop' this means that GEGL was unable to locate any of it's\n"
                         "plug-ins. Try making GEGL_PATH point to the directory containing the .so|.dll\n"
                         "files with the image processing plug-ins, optionally you could try to make it\n"
                         "point to the operations directory of a GEGL sourcetree with a build.");
            }
          return;
        }

      if (node->operation &&
          type == G_OBJECT_TYPE (node->operation) &&
          first_property)
        {
          gegl_node_set_valist (node, first_property, var_args);
          return;
        }

      operation = GEGL_OPERATION (g_object_new_valist (type, first_property,
                                                       var_args));
      gegl_node_set_operation_object (node, operation);
      g_object_unref (operation);
    }
}

static gboolean
gegl_node_invalidate_have_rect (GObject    *gobject,
                                gpointer    foo,
                                gpointer    user_data)
{
  GEGL_NODE (user_data)->valid_have_rect = FALSE;
  return TRUE;
}


static void
gegl_node_property_changed (GObject    *gobject,
                            GParamSpec *arg1,
                            gpointer    user_data)
{
  GeglNode *self = GEGL_NODE (user_data);

  if (self->operation &&
      arg1 != user_data &&
      g_type_is_a (G_OBJECT_TYPE (self->operation), GEGL_TYPE_OPERATION_META))
    {
      gegl_operation_meta_property_changed (
        GEGL_OPERATION_META (self->operation), arg1, user_data);
    }

  if (arg1 != user_data &&
      ((arg1 &&
        arg1->value_type != GEGL_TYPE_BUFFER) ||
       (self->operation && !arg1)))
    {
      if (self->operation && !arg1)
        { /* these means we were called due to a operation change

             FIXME: The logic of this if is not quite intuitive,
             perhaps the thing being checked should be slightly different,
             or perhaps a bug lurks here?
           */
          GeglRectangle dirty_rect;
/*          GeglRectangle new_have_rect;*/

          dirty_rect = self->have_rect;
          /*new_have_rect = gegl_node_get_bounding_box (self);

             gegl_rectangle_bounding_box (&dirty_rect,
                                       &dirty_rect,
                                       &new_have_rect);*/

          gegl_node_invalidated (self, &dirty_rect, FALSE);
        }
      else
        {
          /* we were called due to a property change */
          GeglRectangle dirty_rect;
          GeglRectangle new_have_rect;

          dirty_rect    = self->have_rect;
          new_have_rect = gegl_node_get_bounding_box (self);

          gegl_rectangle_bounding_box (&dirty_rect,
                                       &dirty_rect,
                                       &new_have_rect);

          gegl_node_invalidated (self, &dirty_rect, FALSE);
        }
    }
}

static void
gegl_node_set_operation_object (GeglNode      *self,
                                GeglOperation *operation)
{
  g_return_if_fail (GEGL_IS_NODE (self));

  if (!operation)
    return;

  g_return_if_fail (GEGL_IS_OPERATION (operation));

  {
    GSList   *output_c        = NULL;
    GeglNode *output          = NULL;
    gchar    *output_dest_pad = NULL;
    GSList   *old_pads        = NULL;
    GeglNode *input           = NULL;
    GeglNode *aux             = NULL;

    if (self->operation)
      g_object_unref (self->operation);

    g_object_ref (operation);
    self->operation = operation;

    /* FIXME: handle multiple outputs */

    if (gegl_node_get_pad (self, "output"))
      output_c = gegl_pad_get_connections (gegl_node_get_pad (self, "output"));
    if (output_c && output_c->data)
      {
        GeglConnection *connection = output_c->data;
        GeglPad        *pad;

        output          = gegl_connection_get_sink_node (connection);
        pad             = gegl_connection_get_sink_pad (connection);
        output_dest_pad = g_strdup (pad->param_spec->name);
      }
    input = gegl_node_get_producer (self, "input", NULL);
    aux   = gegl_node_get_producer (self, "aux", NULL);

    gegl_node_disconnect_sources (self);
    gegl_node_disconnect_sinks (self);

    /* Delete all the pads from the previous operation */
    while ((old_pads = gegl_node_get_pads (self)) != NULL)
      {
        gegl_node_remove_pad (self, old_pads->data);
      }

    gegl_operation_attach (operation, self);

    /* FIXME: handle this in a more generic way, but it is needed to allow
     * the attach to work properly.
     */
    if (input)
      gegl_node_connect_from (self, "input", input, "output");
    if (aux)
      gegl_node_connect_from (self, "aux", aux, "output");
    if (output)
      gegl_node_connect_to (self, "output", output, output_dest_pad);

    if (output_dest_pad)
      g_free (output_dest_pad);
  }

  g_signal_connect (G_OBJECT (operation), "notify", G_CALLBACK (gegl_node_invalidate_have_rect), self);
  g_signal_connect (G_OBJECT (operation), "notify", G_CALLBACK (gegl_node_property_changed), self);
  gegl_node_property_changed (G_OBJECT (operation), (GParamSpec *) self, self);
}

void
gegl_node_set (GeglNode    *self,
               const gchar *first_property_name,
               ...)
{
  va_list var_args;

  g_return_if_fail (GEGL_IS_NODE (self));

  va_start (var_args, first_property_name);
  gegl_node_set_valist (self, first_property_name, var_args);
  va_end (var_args);
}

void
gegl_node_get (GeglNode    *self,
               const gchar *first_property_name,
               ...)
{
  va_list var_args;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (self->is_graph || GEGL_IS_OPERATION (self->operation));

  va_start (var_args, first_property_name);
  gegl_node_get_valist (self, first_property_name, var_args);
  va_end (var_args);
}

void
gegl_node_set_valist (GeglNode    *self,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  const gchar *property_name;

  g_return_if_fail (GEGL_IS_NODE (self));

  g_object_ref (self);

  g_object_freeze_notify (G_OBJECT (self));

  property_name = first_property_name;
  while (property_name)
    {
      GValue      value = { 0, };
      GParamSpec *pspec = NULL;
      gchar      *error = NULL;

      if (!strcmp (property_name, "operation"))
        {
          const gchar *op_class;
          const gchar *op_first_property;

          op_class          = va_arg (var_args, gchar *);
          op_first_property = va_arg (var_args, gchar *);

          /* pass the following properties as construction properties
           * to the operation */
          gegl_node_set_op_class (self, op_class, op_first_property, var_args);
          break;
        }
      else if (!strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
            G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT (&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (self), property_name, &value);
          g_value_unset (&value);
        }
      else
        {
          if (self->operation)
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
            }
          if (!pspec)
            {
              g_warning ("%s:%s has no property named: '%s'",
                         G_STRFUNC,
                         gegl_node_get_debug_name (self), property_name);
              break;
            }
          if (!(pspec->flags & G_PARAM_WRITABLE))
            {
              g_warning ("%s: property (%s of operation class '%s' is not writable",
                         G_STRFUNC,
                         pspec->name,
                         G_OBJECT_TYPE_NAME (self->operation));
              break;
            }

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT (&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (self->operation), property_name, &value);
          g_value_unset (&value);
        }

      property_name = va_arg (var_args, gchar *);
    }
  g_object_thaw_notify (G_OBJECT (self));

  g_object_unref (self);
}

void
gegl_node_get_valist (GeglNode    *self,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  const gchar *property_name;

  g_return_if_fail (G_IS_OBJECT (self));

  g_object_ref (self);

  property_name = first_property_name;

  while (property_name)
    {
      GValue      value = { 0, };
      GParamSpec *pspec;
      gchar      *error;

      if (!strcmp (property_name, "operation") ||
          !strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
            G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);
        }
      else
        {
          if (self->is_graph)
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (
                                      gegl_node_get_output_proxy (self, "output")->operation)), property_name);
              if (!pspec)
                {
                  pspec = g_object_class_find_property (
                    G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
                }
            }
          else
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
            }

          if (!pspec)
            {
              g_warning ("%s:%s has no property named: '%s'",
                         G_STRFUNC,
                         gegl_node_get_debug_name (self), property_name);
              break;
            }
          if (!(pspec->flags & G_PARAM_READABLE))
            {
              g_warning ("%s: property '%s' of operation class '%s' is not readable",
                         G_STRFUNC,
                         property_name,
                         G_OBJECT_TYPE_NAME (self->operation));
            }
        }

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      gegl_node_get_property (self, property_name, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      g_value_unset (&value);

      property_name = va_arg (var_args, gchar *);
    }
  g_object_unref (self);
}

void
gegl_node_set_property (GeglNode     *self,
                        const gchar  *property_name,
                        const GValue *value)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  if (!strcmp (property_name, "operation") ||
      !strcmp (property_name, "name"))
    {
      g_object_set_property (G_OBJECT (self),
                             property_name, value);
    }
  else
    {
      if (self->operation)
        {
          g_object_set_property (G_OBJECT (self->operation),
                                 property_name, value);
        }
    }
}

void
gegl_node_get_property (GeglNode    *self,
                        const gchar *property_name,
                        GValue      *value)
{
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  if (!strcmp (property_name, "operation") ||
      !strcmp (property_name, "name"))
    {
      g_object_get_property (G_OBJECT (self),
                             property_name, value);
    }
  else
    {
      if (self->is_graph &&
          !strcmp (property_name, "output"))
        {
          g_warning ("Eeek!");
          g_object_get_property (G_OBJECT (gegl_node_get_output_proxy (self, "output")->operation),
                                 property_name, value);
        }
      else
        {
          if (self->operation)
            {
              g_object_get_property (G_OBJECT (self->operation),
                                     property_name, value);
            }
        }
    }
}

GParamSpec *
gegl_node_find_property (GeglNode    *self,
                         const gchar *property_name)
{
  GParamSpec *pspec = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  if (self->operation)
    pspec = g_object_class_find_property (
      G_OBJECT_GET_CLASS (G_OBJECT (self->operation)), property_name);
  if (!pspec)
    pspec = g_object_class_find_property (
      G_OBJECT_GET_CLASS (G_OBJECT (self)), property_name);
  return pspec;
}

const gchar *
gegl_node_get_operation (const GeglNode *node)
{
  if (node == NULL)
    return NULL;

  if (node->operation == NULL)
    {
      if (node->is_graph)
        return "GraphNode";

      return NULL;
    }

  return GEGL_OPERATION_GET_CLASS (node->operation)->name;
}

void
gegl_node_set_need_rect (GeglNode *node,
                         gpointer  context_id,
                         const GeglRectangle *rect)
{
  GeglOperationContext *context;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (context_id != NULL);

  context = gegl_node_get_context (node, context_id);
  gegl_operation_context_set_need_rect (context, rect);
}

const gchar *
gegl_node_get_debug_name (GeglNode *node)
{
  static gchar  ret_buf[512];

  const gchar  *name;
  const gchar  *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  name      = gegl_node_get_name (node);
  operation = gegl_node_get_operation (node);

  if (name && *name)
    {
      g_snprintf (ret_buf, sizeof (ret_buf),
                  "%s '%s' %p", operation ? operation : "(none)", name, node);
    }
  else
    {
      g_snprintf (ret_buf, sizeof (ret_buf),
                  "%s %p", operation ? operation : "(none)", node);
    }

  return ret_buf;
}

GeglNode *
gegl_node_get_producer (GeglNode *node,
                        gchar    *pad_name,
                        gchar   **output_pad_name)
{
  GeglNode *ret;
  gpointer pad;

  /* XXX: there should be public API to test if a node is
   * really a graph. So that the user of the API knows
   * the internals can be reached through the proxy nops
   */
  if (node->is_graph)
    node = gegl_node_get_input_proxy (node, "input");

  pad = gegl_node_get_pad (node, pad_name);

  if (!pad)
    return NULL;
  pad = gegl_pad_get_connected_to (pad);
  if (!pad)
    return NULL;
  ret = gegl_pad_get_node (pad);

    if(ret)
      {
        const gchar *name;
        name = gegl_node_get_name (ret);
        if (name && !strcmp (name, "proxynop-output"))
          {
            ret = g_object_get_data (G_OBJECT (ret), "graph");
            /* XXX: needs testing whether this returns the correct value
             * for non "output" output pads.
             */
            if (output_pad_name)
              *output_pad_name = g_strdup (gegl_pad_get_name (pad));
          }
        else
          {
            if (output_pad_name)
              *output_pad_name = g_strdup (gegl_pad_get_name (pad));
          }
      }
  return ret;
}

GeglRectangle
gegl_node_get_bounding_box (GeglNode *root)
{
  GeglRectangle dummy = { 0, 0, 0, 0 };
  GeglVisitor  *prepare_visitor;
  GeglVisitor  *have_visitor;
  GeglVisitor  *finish_visitor;

  guchar       *id;
  gint          i;

  GeglPad      *pad;

  if (!root)
    return dummy;

  if (root->valid_have_rect)
    return root->have_rect;

  pad = gegl_node_get_pad (root, "output");
  if (pad && pad->node != root)
    {
      root = pad->node;
    }
  if (!pad || !root)
    return dummy;
  g_object_ref (root);

  id = g_malloc (1);

  for (i = 0; i < 2; i++)
    {
      prepare_visitor = g_object_new (GEGL_TYPE_PREPARE_VISITOR, "id", id, NULL);
      gegl_visitor_dfs_traverse (prepare_visitor, GEGL_VISITABLE (root));
      g_object_unref (prepare_visitor);
    }

  have_visitor = g_object_new (GEGL_TYPE_HAVE_VISITOR, "id", id, NULL);
  gegl_visitor_dfs_traverse (have_visitor, GEGL_VISITABLE (root));
  g_object_unref (have_visitor);

  finish_visitor = g_object_new (GEGL_TYPE_FINISH_VISITOR, "id", id, NULL);
  gegl_visitor_dfs_traverse (finish_visitor, GEGL_VISITABLE (root));
  g_object_unref (finish_visitor);

  g_object_unref (root);
  g_free (id);

  root->valid_have_rect = TRUE;
  return root->have_rect;
}

#if 1

void
gegl_node_process (GeglNode *self)
{
  /* XXX: should perhaps use the internal processor? */
  GeglProcessor *processor;

  g_return_if_fail (GEGL_IS_NODE (self));

  processor = gegl_node_new_processor (self, NULL);

  while (gegl_processor_work (processor, NULL)) ;
  g_object_unref (processor);
}

#else
/* simplest form of GeglProcess that processes all data in one
 *
 * single large chunk
 */
void
gegl_node_process (GeglNode *self)
{
  GeglNode        *input;
  GeglOperationContext *context;
  GeglBuffer      *buffer;
  GeglRectangle    defined;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (self->operation),
                                 GEGL_TYPE_OPERATION_SINK));

  input   = gegl_node_get_producer (self, "input", NULL);
  defined = gegl_node_get_bounding_box (input);
  buffer  = gegl_node_apply_roi (input, "output", &defined, 3);

  g_assert (GEGL_IS_BUFFER (buffer));
  context = gegl_node_add_context (self, &defined);

  {
    GValue value = { 0, };
    g_value_init (&value, GEGL_TYPE_BUFFER);
    g_value_set_object (&value, buffer);
    gegl_operation_context_set_property (context, "input", &value);
    g_value_unset (&value);
  }

  gegl_operation_context_set_result_rect (context, &defined);
  gegl_operation_process (self->operation, context, "output", &defined);
  gegl_node_remove_context (self, &defined);
  g_object_unref (buffer);
}
#endif

void babl_backtrack (void);

GeglOperationContext *
gegl_node_get_context (GeglNode *self,
                       gpointer  context_id)
{
  GeglOperationContext *context = NULL;
  g_mutex_lock (self->mutex);

  context = g_hash_table_lookup (self->priv->contexts, context_id);
  g_mutex_unlock (self->mutex);
  return context;
}

void
gegl_node_remove_context (GeglNode *self,
                          gpointer  context_id)
{
  GeglOperationContext *context;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (context_id != NULL);

  context = gegl_node_get_context (self, context_id);
  g_mutex_lock (self->mutex);
  if (!context)
    {
      g_warning ("didn't find context %p for %s",
                 context_id, gegl_node_get_debug_name (self));
      g_mutex_unlock (self->mutex);
      return;
    }
  g_hash_table_remove (self->priv->contexts, context_id);
  gegl_operation_context_destroy (context);
  g_mutex_unlock (self->mutex);
}

/* Creates, sets up and returns a new context for the node, or just returns it
 * if it is already set up. Also adds it to an internal hash table.
 */
GeglOperationContext *
gegl_node_add_context (GeglNode *self,
                       gpointer  context_id)
{
  GeglOperationContext *context = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (context_id != NULL, NULL);

  g_mutex_lock (self->mutex);
  context = g_hash_table_lookup (self->priv->contexts, context_id);

  if (context)
    {
      /* silently ignore, since multiple traversals of prepare are done
       * to saturate the graph */
      g_mutex_unlock (self->mutex);
      return context;
    }

  context             = gegl_operation_context_new ();
  context->operation  = self->operation;
  g_hash_table_insert (self->priv->contexts, context_id, context);
  g_mutex_unlock (self->mutex);
  return context;
}

GeglNode *
gegl_node_detect (GeglNode *root,
                  gint      x,
                  gint      y)
{
  if (root)
    {
      /* make sure the have rects are computed */
      /* FIXME: do not call this all the time! */
      gegl_node_get_bounding_box (root);

      if (root->operation)
        return gegl_operation_detect (root->operation, x, y);
      else
        {
          if (root->is_graph)
            {
              GeglNode *foo = gegl_node_get_output_proxy (root, "output");
              if (foo && foo != root)
                return gegl_node_detect (foo, x, y);
            }
        }
    }
  return root;
}

/* this is a bit hacky, but allows us to insert a node into the graph,
 * and avoid the full defined region of the entire graph to change
 */
void
gegl_node_insert_before (GeglNode *self,
                         GeglNode *to_be_inserted)
{
  GeglNode     *other;
  GeglRectangle rectangle;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_NODE (to_be_inserted));

  other     = gegl_node_get_producer (self, "input", NULL);/*XXX: handle pad name */
  rectangle = gegl_node_get_bounding_box (to_be_inserted);

  g_signal_handlers_block_matched (other, G_SIGNAL_MATCH_FUNC, 0, 0, 0, gegl_node_source_invalidated, NULL);
  /* the blocked handler disappears during the relinking */
  gegl_node_link_many (other, to_be_inserted, self, NULL);

  /* emit the change ourselves */
  gegl_node_invalidated (self, &rectangle, FALSE);
}

gint
gegl_node_get_consumers (GeglNode      *node,
                         const gchar   *output_pad,
                         GeglNode    ***nodes,
                         const gchar ***pads)
{
  GSList  *connections;
  gint     n_connections;
  GeglPad *pad;
  gchar  **pasp = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (node), 0);
  g_return_val_if_fail (output_pad != NULL, 0);

  pad = gegl_node_get_pad (node, output_pad);

  if (!pad)
    {
      g_warning ("%s: no such pad %s for %s",
                 G_STRFUNC, output_pad, gegl_node_get_debug_name (node));
      return 0;
    }

  connections = gegl_pad_get_connections (pad);
  {
    GSList *iter;
    gint    pasp_size = 0;
    gint    i;
    gint    pasp_pos = 0;

    n_connections = g_slist_length (connections);
    pasp_size    += (n_connections + 1) * sizeof (gchar *);

    for (iter = connections; iter; iter = g_slist_next (iter))
      {
        GeglConnection *connection = iter->data;
        GeglPad        *pad        = gegl_connection_get_sink_pad (connection);
        pasp_size += strlen (gegl_pad_get_name (pad)) + 1;
      }
    if (nodes)
      *nodes = g_malloc ((n_connections + 1) * sizeof (void *));
    if (pads)
      {
        pasp  = g_malloc (pasp_size);
        *pads = (void *) pasp;
      }
    i        = 0;
    pasp_pos = (n_connections + 1) * sizeof (void *);
    for (iter = connections; iter; iter = g_slist_next (iter))
      {
        GeglConnection *connection = iter->data;
        GeglPad        *pad        = gegl_connection_get_sink_pad (connection);
        GeglNode       *node       = gegl_connection_get_sink_node (connection);
        const gchar    *pad_name   = gegl_pad_get_name (pad);

        if (nodes)
          (*nodes)[i] = node;
        if (pasp)
          {
            pasp[i] = ((gchar *) pasp) + pasp_pos;
            strcpy (pasp[i], pad_name);
          }
        pasp_pos += strlen (pad_name) + 1;
        i++;
      }
    if (nodes)
      (*nodes)[i] = NULL;
    if (pads)
      pasp[i] = NULL;
  }
  return n_connections;
}


void
gegl_node_emit_computed (GeglNode *node,
                         const GeglRectangle *rect)
{
  static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
  g_static_mutex_lock (&mutex);
  g_signal_emit (node, gegl_node_signals[COMPUTED], 0, rect, NULL, NULL);
  g_static_mutex_unlock (&mutex);
}

static void
gegl_node_computed_event (GeglCache *self,
                          void      *foo,
                          void      *user_data)
{
  GeglNode *node = GEGL_NODE (user_data);
  gegl_node_emit_computed (node, foo);
}

GeglCache *
gegl_node_get_cache (GeglNode *node)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  if (!node->cache)
    {
      GeglPad    *pad;
      const Babl *format;

      /* XXX: it should be possible to have cache for other pads than
       * only "output" pads
       */
      pad = gegl_node_get_pad (node, "output");
      if (!pad)
        return NULL;
      format = gegl_pad_get_format (pad);
      if (!format)
        {
          format = babl_format ("RGBA float");
        }

      node->cache = g_object_new (GEGL_TYPE_CACHE,
                                  "node", node,
                                  "format", format,
                                  NULL);
      g_signal_connect (G_OBJECT (node->cache), "computed",
                        (GCallback) gegl_node_computed_event,
                        node);
    }
  return node->cache;
}

const gchar *
gegl_node_get_name (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->priv->name;
}

void
gegl_node_set_name (GeglNode    *self,
                    const gchar *name)
{
  g_return_if_fail (GEGL_IS_NODE (self));

  if (self->priv->name)
    g_free (self->priv->name);
  self->priv->name = g_strdup (name);
}

void
gegl_node_remove_children (GeglNode *self)
{
  g_return_if_fail (GEGL_IS_NODE (self));

  while (TRUE)
    {
      GeglNode *child = gegl_node_get_nth_child (self, 0);

      if (child && GEGL_IS_NODE (child))
        gegl_node_remove_child (self, child);
      else
        break;
    }
}

GeglNode *
gegl_node_add_child (GeglNode *self,
                     GeglNode *child)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (child), NULL);
  g_return_val_if_fail (child->priv->parent == NULL, NULL);

  self->priv->children = g_slist_prepend (self->priv->children,
                                          g_object_ref (child));
  self->is_graph      = TRUE;
  child->priv->parent = self;

  child->dont_cache = self->dont_cache;

  return child;
}

GeglNode *
gegl_node_remove_child (GeglNode *self,
                        GeglNode *child)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);
  if (!GEGL_IS_NODE (child))
    {
      g_print ("%p %s\n", child, G_OBJECT_TYPE_NAME (child));
    }
  g_return_val_if_fail (GEGL_IS_NODE (child), NULL);

  g_assert (child->priv->parent == self ||
            child->priv->parent == NULL);

  self->priv->children = g_slist_remove (self->priv->children, child);

  if (child->priv->parent != NULL)
    {
      /* if parent isn't set then the node is already in dispose
       */
      child->priv->parent = NULL;
      g_object_unref (child);
    }

  if (self->priv->children == NULL)
    self->is_graph = FALSE;

  return child;
}

GeglNode *
gegl_node_get_parent (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return self->priv->parent;
}

gint
gegl_node_get_num_children (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), -1);

  return g_slist_length (self->priv->children);
}

GeglNode *
gegl_node_get_nth_child (GeglNode *self,
                         gint      n)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return g_slist_nth_data (self->priv->children, n);
}

/*
 * Returns a copy of the graphs internal list of nodes
 */
GSList *
gegl_node_get_children (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return g_slist_copy (self->priv->children);
}

/*
 *  returns a freshly created node, owned by the graph, and thus freed with it
 */
GeglNode *
gegl_node_new_child (GeglNode    *parent,
                     const gchar *first_property_name,
                     ...)
{
  GeglNode    *node;
  va_list      var_args;
  const gchar *name;

  node = g_object_new (GEGL_TYPE_NODE, NULL);
  if (parent)
    {
      gegl_node_add_child (parent, node);
    }

  name = first_property_name;
  va_start (var_args, first_property_name);
  gegl_node_set_valist (node, name, var_args);
  va_end (var_args);

  if (parent)
    g_object_unref (node);
  return node;
}

GeglNode *
gegl_node_create_child (GeglNode    *self,
                        const gchar *operation)
{
  GeglNode *ret;
  g_return_val_if_fail (operation != NULL, NULL);

  ret = gegl_node_new_child (self, "operation", operation, NULL);
  if (ret && self)
    {
      ret->dont_cache = self->dont_cache;
    }
  return ret;
}

static void
graph_source_invalidated (GeglNode            *source,
                          const GeglRectangle *rect,
                          gpointer             data)
{
  GeglNode      *destination = GEGL_NODE (data);
  GeglRectangle  dirty_rect  = *rect;

  GEGL_NOTE (GEGL_DEBUG_INVALIDATION, "graph:%s is dirtied from %s (%i,%i %ix%i)",
             gegl_node_get_debug_name (destination),
             gegl_node_get_debug_name (source),
             rect->x, rect->y,
             rect->width, rect->height);

  g_signal_emit (destination, gegl_node_signals[INVALIDATED], 0,
                 &dirty_rect, NULL);
}


static GeglNode *
gegl_node_get_pad_proxy (GeglNode    *graph,
                         const gchar *name,
                         gboolean     is_graph_input)
{
  GeglNode *node = graph;
  GeglPad  *pad;

  pad = gegl_node_get_pad (node, name);
  if (!pad)
    {
      GeglNode *nop      = NULL;
      GeglPad  *nop_pad  = NULL;
      gchar    *nop_name = NULL;

      nop_name = g_strconcat ("proxynop-", name, NULL);
      nop      = g_object_new (GEGL_TYPE_NODE, "operation", "gegl:nop", "name", nop_name, NULL);
      nop_pad  = gegl_node_get_pad (nop, is_graph_input ? "input" : "output");
      g_free (nop_name);

      gegl_node_add_child (graph, nop);
      g_object_unref (nop); /* our reference is made by the
                               gegl_node_add_child call */

      {
        GeglPad *new_pad = g_object_new (GEGL_TYPE_PAD, NULL);
        gegl_pad_set_param_spec (new_pad, nop_pad->param_spec);
        gegl_pad_set_node (new_pad, nop);
        gegl_pad_set_name (new_pad, name);
        gegl_node_add_pad (node, new_pad);
      }

      g_object_set_data (G_OBJECT (nop), "graph", graph);

      if (!is_graph_input)
        {
          g_signal_connect (G_OBJECT (nop), "invalidated",
                            G_CALLBACK (graph_source_invalidated), graph);
        }
      return nop;
    }
  return gegl_pad_get_node (pad);
}

GeglNode *
gegl_node_get_input_proxy (GeglNode    *node,
                           const gchar *name)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  return gegl_node_get_pad_proxy (node, name, TRUE);
}

GeglNode *
gegl_node_get_output_proxy (GeglNode    *node,
                            const gchar *name)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  return gegl_node_get_pad_proxy (node, name, FALSE);
}

GeglNode *
gegl_node_new (void)
{
  return g_object_new (GEGL_TYPE_NODE, NULL);
}
