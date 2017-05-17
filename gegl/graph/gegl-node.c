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
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-node-private.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"
#include "gegl-config.h"

#include "graph/gegl-visitor.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operations.h"
#include "operation/gegl-operation-meta.h"

#include "process/gegl-eval-manager.h"

enum
{
  PROP_0,
  PROP_OP_CLASS,
  PROP_OPERATION,
  PROP_NAME,
  PROP_DONT_CACHE,
  PROP_USE_OPENCL,
  PROP_PASSTHROUGH
};

enum
{
  INVALIDATED,
  COMPUTED,
  PROGRESS,
  LAST_SIGNAL
};


struct _GeglNodePrivate
{
  GSList          *source_connections;
  GSList          *sink_connections;
  GSList          *children;  /*  used for children */
  GeglNode        *parent;
  gchar           *name;
  gchar           *debug_name;
  GeglEvalManager *eval_manager;
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

static void            gegl_node_update_debug_name        (GeglNode *node);


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
                                                        FALSE,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_USE_OPENCL,
                                   g_param_spec_boolean ("use-opencl",
                                                         "Use OpenCL",
                                                         "Use the OpenCL version of this operation if available, this property is inherited by children created from a node.",
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The name of the node",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PASSTHROUGH,
                                   g_param_spec_boolean ("passthrough",
                                                         "Passthrough",
                                                        "Act as a nop, passing input unmodifed through to ouput.",
                                                        FALSE,
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

  gegl_node_signals[PROGRESS] =
    g_signal_new ("progress",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

static void
gegl_node_init (GeglNode *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            GEGL_TYPE_NODE,
                                            GeglNodePrivate);

  self->pads        = NULL;
  self->input_pads  = NULL;
  self->output_pads = NULL;
  self->operation   = NULL;
  self->is_graph    = FALSE;
  self->cache       = NULL;
  g_mutex_init (&self->mutex);

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

  if (self->priv->eval_manager)
    {
      g_object_unref (self->priv->eval_manager);
      self->priv->eval_manager = NULL;
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

  if (self->priv->debug_name)
    {
      g_free (self->priv->debug_name);
    }

  g_mutex_clear (&self->mutex);

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

      case PROP_PASSTHROUGH:
        node->passthrough = g_value_get_boolean (value);
        break;

      case PROP_USE_OPENCL:
        node->use_opencl = g_value_get_boolean (value);
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

      case PROP_PASSTHROUGH:
        g_value_set_boolean (value, node->passthrough);
        break;

      case PROP_USE_OPENCL:
        g_value_set_boolean (value, node->use_opencl);
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

static inline gchar **
_make_pad_list (GSList *iter)
{
  gchar **list;
  gint    i;

  if (!iter)
    return NULL;

  list = g_new0 (gchar *, g_slist_length (iter) + 1);

  for (i = 0; iter; iter = iter->next, i++)
    {
      GeglPad *pad = iter->data;
      list[i] = g_strdup (pad->name);
    }

  return list;
}

gchar **
gegl_node_list_input_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return _make_pad_list (self->input_pads);
}

gchar **
gegl_node_list_output_pads (GeglNode *self)
{
  g_return_val_if_fail (GEGL_IS_NODE (self), NULL);

  return _make_pad_list (self->output_pads);
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
  GeglNode *pad_node;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (GEGL_IS_PAD (pad));

  self->pads = g_slist_remove (self->pads, pad);

  if (gegl_pad_is_output (pad))
    self->output_pads = g_slist_remove (self->output_pads, pad);

  if (gegl_pad_is_input (pad))
    self->input_pads = g_slist_remove (self->input_pads, pad);

  pad_node = gegl_pad_get_node (pad);

  /* This was a proxy pad, also remove the nop node */
  if (self != pad_node)
    gegl_node_remove_child (self, pad_node);

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

static GSList *
gegl_node_get_depends_on (GeglNode *self);

static gboolean
gegl_node_has_source (GeglNode *self,
                      GeglNode *potential_source)
{
  GSList *producers, *p;
  gboolean found = FALSE;

  if (self == potential_source)
    return TRUE;

  producers = gegl_node_get_depends_on (self);
  for (p = producers; p; p = p->next)
    {
      if (p->data == potential_source)
        found = TRUE;
      else
        found = gegl_node_has_source (p->data, potential_source);
      if (found)
        break;
    }
  g_slist_free (producers);

  return found;
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

  if (gegl_node_has_source (source, sink))
    {
      g_warning ("Construction of loop requested, bailing\n");
      return FALSE;
    }

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

  if (gegl_node_pads_exist (real_sink, real_sink_pad_name, real_source, real_source_pad_name))
    {
      GeglPad        *other_pad;
      GeglPad        *sink_pad   = gegl_node_get_pad (real_sink, real_sink_pad_name);
      GeglPad        *source_pad = gegl_node_get_pad (real_source, real_source_pad_name);
      GeglConnection *connection;

      other_pad = gegl_pad_get_connected_to (sink_pad);
      if (source_pad == other_pad)
        return TRUE;

      gegl_node_disconnect (real_sink, real_sink_pad_name);

      connection = gegl_pad_connect (sink_pad, source_pad);
      gegl_connection_set_sink_node (connection, real_sink);
      gegl_connection_set_source_node (connection, real_source);

      real_sink->priv->source_connections = g_slist_prepend (real_sink->priv->source_connections, connection);
      real_source->priv->sink_connections = g_slist_prepend (real_source->priv->sink_connections, connection);

      g_signal_connect (G_OBJECT (real_source), "invalidated",
                        G_CALLBACK (gegl_node_source_invalidated), sink_pad);

      gegl_node_source_invalidated (real_source, &real_source->have_rect, sink_pad);

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

static GeglEvalManager *
gegl_node_get_eval_manager (GeglNode *self)
{
  if (!self->priv->eval_manager)
    self->priv->eval_manager = gegl_eval_manager_new (self, "output");
  return self->priv->eval_manager;
}

static GeglBuffer *
gegl_node_apply_roi (GeglNode            *self,
                     const GeglRectangle *roi,
                     gint                 level)
{
  GeglEvalManager *eval_manager = gegl_node_get_eval_manager (self);

  if (roi)
    {
      return gegl_eval_manager_apply (eval_manager, roi, level);
    }
  else
    {
      GeglRectangle node_bbox = gegl_node_get_bounding_box (self);
      return gegl_eval_manager_apply (eval_manager, &node_bbox, level);
    }
}

void
gegl_node_blit_buffer (GeglNode            *self,
                       GeglBuffer          *buffer,
                       const GeglRectangle *roi,
                       gint                 level,
                       GeglAbyssPolicy      abyss_policy) 
{
  // XXX: make use of abyss_policy

  GeglEvalManager *eval_manager;
  GeglBuffer      *result;
  GeglRectangle    request;

  eval_manager = gegl_node_get_eval_manager (self);

  if (roi)
    request = *roi;
  else if (buffer)
    request = *gegl_buffer_get_extent (buffer);
  else
    request = gegl_node_get_bounding_box (self);

  result = gegl_eval_manager_apply (eval_manager, &request, level);

  if (result)
    {
      if (buffer)
        gegl_buffer_copy (result, &request, GEGL_ABYSS_NONE, buffer, NULL);
      g_object_unref (result);
    }
}

static inline gboolean gegl_mipmap_rendering_enabled (void)
{
  static int enabled = -1;
  if (enabled == -1)
    enabled = g_getenv("GEGL_MIPMAP_RENDERING")!=NULL;
  return enabled;
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
  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (roi != NULL);

  if (rowstride == GEGL_AUTO_ROWSTRIDE && format)
    rowstride = babl_format_get_bytes_per_pixel (format) * roi->width;

  if (!flags)
    {
      GeglBuffer *buffer;

      if (scale != 1.0)
        {
          const GeglRectangle unscaled_roi = _gegl_get_required_for_scale (format, roi, scale);

          buffer = gegl_node_apply_roi (self, &unscaled_roi,
              gegl_mipmap_rendering_enabled()?gegl_level_from_scale (scale):0);
        }
      else
        {
          buffer = gegl_node_apply_roi (self, roi, 0);
        }
      if (buffer && destination_buf)
        gegl_buffer_get (buffer, roi, scale, format, destination_buf, rowstride, GEGL_ABYSS_NONE);

      if (buffer)
        g_object_unref (buffer);
    }
  else if (flags & GEGL_BLIT_CACHE)
    {
      GeglCache  *cache  = gegl_node_get_cache (self);
      GeglBuffer *buffer = GEGL_BUFFER (cache);

      if (!(flags & GEGL_BLIT_DIRTY))
        {
          if (scale != 1.0)
            {
              const GeglRectangle unscaled_roi = _gegl_get_required_for_scale (format, roi, scale);
              gint  level = gegl_mipmap_rendering_enabled()?gegl_level_from_scale (scale):0;

              gegl_node_blit_buffer (self, buffer, &unscaled_roi, level, GEGL_ABYSS_NONE);
              gegl_cache_computed (cache, &unscaled_roi, level);
            }
          else
            {
              gegl_node_blit_buffer (self, buffer, roi, 0, GEGL_ABYSS_NONE);
              gegl_cache_computed (cache, roi, 0);
            }
        }

      if (destination_buf && cache)
        {
          gegl_buffer_get (buffer, roi, scale,
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
/*        GeglRectangle new_have_rect;*/

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

  if (arg1)
    g_object_notify_by_pspec (G_OBJECT (self), arg1);
}

static void
gegl_node_set_operation_object (GeglNode      *self,
                                GeglOperation *operation)
{
  GeglNode    **consumer_nodes = NULL;
  const gchar **consumer_names = NULL;
  GeglNode    *input           = NULL;
  GeglNode    *aux             = NULL;
  GeglNode    *aux2            = NULL;

  g_return_if_fail (GEGL_IS_NODE (self));

  if (!operation)
    return;

  g_return_if_fail (GEGL_IS_OPERATION (operation));

  if (gegl_node_has_pad (self, "output"))
    gegl_node_get_consumers (self, "output", &consumer_nodes, &consumer_names);

  input = gegl_node_get_producer (self, "input", NULL);
  aux   = gegl_node_get_producer (self, "aux", NULL);
  aux2  = gegl_node_get_producer (self, "aux2", NULL);

  gegl_node_disconnect_sources (self);
  gegl_node_disconnect_sinks (self);

  if (self->operation)
    g_object_unref (self->operation);

  self->operation = g_object_ref (operation);

  /* Delete all the pads from the previous operation */
  while (self->pads)
    gegl_node_remove_pad (self, self->pads->data);

  gegl_operation_attach (operation, self);

  /* FIXME: This should handle all input pads instead of just these 3 */
  if (input)
    gegl_node_connect_from (self, "input", input, "output");
  if (aux)
    gegl_node_connect_from (self, "aux", aux, "output");
  if (aux2)
    gegl_node_connect_from (self, "aux2", aux2, "output");

  if (consumer_nodes)
    {
      for (gint i = 0; consumer_nodes[i]; ++i)
        {
          GeglNode    *output          = consumer_nodes[i];
          const gchar *output_dest_pad = consumer_names[i];

          if (output)
            gegl_node_connect_to (self, "output", output, output_dest_pad);
        }

      g_free (consumer_nodes);
      g_free (consumer_names);
    }

  g_signal_connect (G_OBJECT (operation), "notify", G_CALLBACK (gegl_node_invalidate_have_rect), self);
  g_signal_connect (G_OBJECT (operation), "notify", G_CALLBACK (gegl_node_property_changed), self);

  gegl_node_update_debug_name (self);

  gegl_node_property_changed (G_OBJECT (operation), (GParamSpec *) NULL, self);
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

  g_object_freeze_notify (G_OBJECT (self));

  if (self->operation)
    g_object_freeze_notify (G_OBJECT (self->operation));

  property_name = first_property_name;
  while (property_name)
    {
      if (!strcmp (property_name, "operation"))
        {
          const gchar *op_class;
          const gchar *op_first_property;

          op_class          = va_arg (var_args, gchar *);
          op_first_property = va_arg (var_args, gchar *);

          if (self->operation)
            g_object_thaw_notify (G_OBJECT (self->operation));

          /* pass the following properties as construction properties
           * to the operation */
          gegl_node_set_op_class (self, op_class, op_first_property, var_args);

          if (self->operation)
            g_object_freeze_notify (G_OBJECT (self->operation));

          break;
        }
      else
        {
          GValue      value  = { 0, };
          GParamSpec *pspec  = NULL;
          gchar      *error  = NULL;
          GObject    *object = NULL;

          pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self),
                                                property_name);
          if (pspec)
            {
              object = G_OBJECT (self);
            }
          else if (self->operation)
            {
              pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self->operation),
                                                    property_name);
              if (pspec)
                object = G_OBJECT (self->operation);
            }

          if (!object)
            {
              g_warning ("%s is not a valid property of %s",
                         property_name,
                         gegl_node_get_debug_name (self));
              break;
            }
          else
            {
              g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
              G_VALUE_COLLECT (&value, var_args, 0, &error);
              if (error)
                {
                  g_warning ("%s: %s", G_STRFUNC, error);
                  g_free (error);
                  g_value_unset (&value);
                  break;
                }
              g_object_set_property (object, property_name, &value);
              g_value_unset (&value);
            }
        }

      property_name = va_arg (var_args, gchar *);
    }

  if (self->operation)
    g_object_thaw_notify (G_OBJECT (self->operation));

  g_object_thaw_notify (G_OBJECT (self));
}

void
gegl_node_get_valist (GeglNode    *self,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  const gchar *property_name;

  g_return_if_fail (G_IS_OBJECT (self));

  property_name = first_property_name;

  while (property_name)
    {
      GValue      value = { 0, };
      gchar      *error;

      gegl_node_get_property (self, property_name, &value);
      if (!G_IS_VALUE (&value))
        break;

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
}

void
gegl_node_set_property (GeglNode     *self,
                        const gchar  *property_name,
                        const GValue *value)
{
  GParamSpec *pspec;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), property_name);
  if (pspec)
    {
      g_object_set_property (G_OBJECT (self), property_name, value);
      return;
    }

  if (self->operation)
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self->operation), property_name);
  if (pspec)
    {
      if (G_IS_PARAM_SPEC_ENUM (pspec) && G_VALUE_HOLDS (value, G_TYPE_STRING))
        {
          GEnumClass  *enum_class   = G_PARAM_SPEC_ENUM (pspec)->enum_class;
          const gchar *value_string = g_value_get_string (value);
          GEnumValue  *enum_value   = NULL;

          enum_value = g_enum_get_value_by_name (enum_class, value_string);
          if (!enum_value)
            enum_value = g_enum_get_value_by_nick (enum_class, value_string);

          if (enum_value)
            {
              GValue value = G_VALUE_INIT;
              g_value_init (&value, G_TYPE_FROM_CLASS (&enum_class->g_type_class));
              g_value_set_enum (&value, enum_value->value);
              g_object_set_property (G_OBJECT (self->operation), property_name, &value);
              g_value_unset (&value);
              return;
            }

          g_warning ("Could not convert %s to a valid enum value for %s",
                     value_string,
                     property_name);
        }
      g_object_set_property (G_OBJECT (self->operation), property_name, value);
      return;
    }

  g_warning ("%s is not a valid property of %s",
             property_name,
             gegl_node_get_debug_name (self));
}

void
gegl_node_get_property (GeglNode    *self,
                        const gchar *property_name,
                        GValue      *value)
{
  GParamSpec *pspec;

  g_return_if_fail (GEGL_IS_NODE (self));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  /* Unlinke GObject's get_property this function also
   * accepts a zero'd GValue and will fill it with the
   * correct type automaticaly.
   */

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), property_name);
  if (pspec)
    {
      if (!G_IS_VALUE (value))
        g_value_init (value, pspec->value_type);
      g_object_get_property (G_OBJECT (self), property_name, value);
      return;
    }

  if (self->operation)
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self->operation), property_name);
  if (pspec)
    {
      if (!G_IS_VALUE (value))
        g_value_init (value, pspec->value_type);
      g_object_get_property (G_OBJECT (self->operation), property_name, value);
      return;
    }

  g_warning ("%s is not a valid property of %s",
             property_name,
             gegl_node_get_debug_name (self));
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

GeglOperation *
gegl_node_get_gegl_operation (GeglNode *node)
{
  if (node == NULL)
    return NULL;

  return node->operation;
}

static void
gegl_node_update_debug_name (GeglNode *node)
{
  const gchar  *name = gegl_node_get_name (node);
  const gchar  *operation = gegl_node_get_operation (node);
  gchar        *new_name = NULL;

  g_return_if_fail (GEGL_IS_NODE (node));

  if (node->priv->debug_name)
    g_free (node->priv->debug_name);

  if (name && *name)
    new_name = g_strdup_printf ("%s '%s' %p", operation ? operation : "(none)", name, node);
  else
    new_name = g_strdup_printf ("%s %p", operation ? operation : "(none)", node);

  node->priv->debug_name = new_name;
}

const gchar *
gegl_node_get_debug_name (GeglNode *node)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  return node->priv->debug_name;
}

GeglNode *
gegl_node_get_producer (GeglNode     *node,
                        const gchar  *pad_name,
                        gchar       **output_pad_name)
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
gegl_node_get_bounding_box (GeglNode *self)
{
  if (!self->valid_have_rect)
    {
      /* We need to construct an evaluation here because we need
       * to be sure we use the same logic as the eval manager to
       * calculate our bounds. We set our rect explicitly because
       * if we're a meta-op the eval manager may not actually
       * visit us in prepare.
       */

      GeglEvalManager *eval = gegl_eval_manager_new (self, "output");
      self->have_rect = gegl_eval_manager_get_bounding_box (eval);
      self->valid_have_rect = TRUE;
      g_object_unref (eval);
    }

  return self->have_rect;
}

void
gegl_node_process (GeglNode *self) /* XXX: add level argument?  */
{
  GeglProcessor *processor;

  g_return_if_fail (GEGL_IS_NODE (self));

  processor = gegl_node_new_processor (self, NULL);

  while (gegl_processor_work (processor, NULL)) ;
  g_object_unref (processor);
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

  other     = gegl_node_get_producer (self, "input", NULL); /*XXX: handle pad name */
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

  g_return_val_if_fail (output_pad != NULL, 0);

  if(node->is_graph)
    node = gegl_node_get_output_proxy(node, "output");

  g_return_val_if_fail (GEGL_IS_NODE (node), 0);

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
        GeglConnection  *connection = iter->data;
        GeglPad                 *pad        = gegl_connection_get_sink_pad (connection);
        GeglNode        *node       = gegl_connection_get_sink_node (connection);
        const gchar     *pad_name   = gegl_pad_get_name (pad);
        const gchar     *name       = gegl_node_get_name(node);

        gchar* proxy_name = g_strconcat("proxynop-", pad_name, NULL);
        if(!strcmp(name, proxy_name))
          {
            node = g_object_get_data(G_OBJECT(node), "graph");
            name = gegl_node_get_name(node);
          }
        else
          {
          }
        g_free (proxy_name);

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
  g_signal_emit (node, gegl_node_signals[COMPUTED], 0, rect, NULL, NULL);
}

GeglCache *
gegl_node_get_cache (GeglNode *node)
{
  GeglPad    *pad;
  GeglNode   *real_node;
  const Babl *format = NULL;
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  pad = gegl_node_get_pad (node, "output");
  g_return_val_if_fail (pad, NULL);

  real_node = gegl_pad_get_node (pad);

  if (node != real_node)
    return gegl_node_get_cache (real_node);

  format = gegl_pad_get_format (pad);

  if (!format)
    {
      g_warning ("Output of %s has no format", gegl_node_get_debug_name (node));

      format = babl_format ("RGBA float");
    }

  if (node->cache && gegl_buffer_get_format ((GeglBuffer *)(node->cache)) != format)
    {
      g_object_unref (node->cache);
      node->cache = NULL;
    }

  if (node->cache)
    return node->cache;

  g_mutex_lock (&node->mutex);

  if (!node->cache)
    {
      GeglCache *cache;

      cache = g_object_new (GEGL_TYPE_CACHE,
                            "format", format,
                            NULL);

      gegl_object_set_has_forked (G_OBJECT (cache));
      gegl_node_get_bounding_box (node);
      gegl_buffer_set_extent (GEGL_BUFFER (cache), &node->have_rect);

      g_signal_connect_swapped (G_OBJECT (cache), "computed",
                                (GCallback) gegl_node_emit_computed,
                                node);
      node->cache = cache;
    }

  g_mutex_unlock (&node->mutex);

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

  gegl_node_update_debug_name (self);
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
  child->use_opencl = self->use_opencl;

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
      ret->use_opencl = self->use_opencl;
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

  gegl_node_invalidated (destination, &dirty_rect, FALSE);
}


static GeglNode *
gegl_node_get_pad_proxy (GeglNode    *node,
                         const gchar *name,
                         gboolean     is_graph_input)
{
  GeglPad *pad = gegl_node_get_pad (node, name);

  if (!pad)
    {
      GeglNode *nop      = NULL;
      GeglPad  *nop_pad  = NULL;
      gchar    *nop_name = NULL;

      nop_name = g_strconcat ("proxynop-", name, NULL);
      nop      = g_object_new (GEGL_TYPE_NODE, "operation", "gegl:nop", "name", nop_name, NULL);
      nop_pad  = gegl_node_get_pad (nop, is_graph_input ? "input" : "output");
      g_free (nop_name);

      gegl_node_add_child (node, nop);
      g_object_unref (nop); /* our reference is made by the
                               gegl_node_add_child call */

      {
        GeglPad *new_pad = g_object_new (GEGL_TYPE_PAD, NULL);
        gegl_pad_set_param_spec (new_pad, nop_pad->param_spec);
        gegl_pad_set_node (new_pad, nop);
        gegl_pad_set_name (new_pad, name);
        gegl_node_add_pad (node, new_pad);
      }

      g_object_set_data (G_OBJECT (nop), "graph", node);

      if (!is_graph_input)
        {
          g_signal_connect_object (G_OBJECT (nop), "computed",
                                   G_CALLBACK (gegl_node_emit_computed), node,
                                   G_CONNECT_SWAPPED);
          g_signal_connect_object (G_OBJECT (nop), "invalidated",
                                   G_CALLBACK (graph_source_invalidated), node,
                                   0);
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

gboolean
gegl_node_get_passthrough (GeglNode *node)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  return node->passthrough;
}

void
gegl_node_set_passthrough (GeglNode *node,
                           gboolean  passthrough)
{
  g_return_if_fail (GEGL_IS_NODE (node));

  if (node->passthrough == passthrough)
    return;

  node->passthrough = passthrough;
  gegl_node_invalidated (node, NULL, TRUE);
}

typedef struct Closure {
  GeglNode  *node;
  gdouble    progress;
} Closure;

static gboolean delayed_emission (void *data)
{
  Closure *closure = data;
  g_signal_emit (closure->node,
                 gegl_node_signals[PROGRESS], 0,
                 closure->progress, NULL, NULL);
  g_free (closure);
  return FALSE;
}

/* this causes dispatch of the signal on the main thread - if we
 * are in the main thread the callback will be directly executed now
 * instead of queued
 */
void gegl_node_progress (GeglNode *node,
                         gdouble   progress,
                         gchar    *message)
{
  if (gegl_is_main_thread ())
    g_signal_emit (node, gegl_node_signals[PROGRESS], 0, progress, message, NULL);
  else
  {
    Closure *closure = g_new0 (Closure, 1);
    closure->node = node;
    closure->progress = progress;
    g_idle_add (delayed_emission, closure);
  }
}

const char *gegl_operation_get_op_version (const char *op_name)
{
  const gchar *ret = gegl_operation_get_key (op_name, "op-version");
  if (!ret)
    ret = "0:0";
  return ret;
}

